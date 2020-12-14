#include <cons.h>
#include <mempool.h>
#include <parser.h>
#include <vcpkgparser.h>

using namespace rlisp;

static void mark(Cons* c, std::vector<bool>& flags, Cons* pool_base)
{
    do
    {
        auto i = c - pool_base;
        if (i < 0 || static_cast<size_t>(i) >= flags.size() || flags[i]) return;
        flags[i] = true;

        // atoms are all separately allocated
        if (c->car == atom_tag) abort();
        mark(c->car, flags, pool_base);
        c = c->cdr;
    } while (1);
}

Cons* MemPool::alloc(Cons* a, Cons* b)
{
    if (m_free_list != nullptr)
    {
        auto c = m_free_list;
        m_free_list = c->cdr;
        c->car = a;
        c->cdr = b;
        return c;
    }
    else if (cur_value < m_cells_size)
    {
        auto& c = m_cells[cur_value++];
        c.car = a;
        c.cdr = b;
        return &c;
    }
    else
    {
        // no free list and entire memory is allocated

        // gc time
        std::vector<bool> flags(m_cells_size, false);
        mark(a, flags, m_cells.get());
        mark(b, flags, m_cells.get());
        for (auto&& root : m_roots)
            mark(root, flags, m_cells.get());

        // sweep
        for (size_t i = flags.size(); i > 0; --i)
        {
            if (!flags[i - 1])
            {
                m_cells[i - 1].cdr = m_free_list;
                // flood car with CC to improve debugging
                memset(&m_cells[i - 1].car, 0xCC, sizeof(m_cells[i - 1].car));
                m_free_list = &m_cells[i - 1];
            }
        }
        if (m_free_list == nullptr)
        {
            // oom
            return nullptr;
        }
        else
        {
            auto c = m_free_list;
            m_free_list = c->cdr;
            c->car = a;
            c->cdr = b;
            return c;
        }
    }
}

Cons* MemPool::nil()
{
    if (!m_nil.atom)
    {
        m_nil.atom = &m_nil_str;
        m_nil_str = "nil";
    }
    return &m_nil;
}

Cons* MemPool::intern_atom(std::string sv)
{
    if (sv == "nil") return nil();

    auto [it, b] = atoms.emplace(std::move(sv), Cons{atom_tag, nullptr});
    if (b)
    {
        it->second.atom = &it->first;
    }
    return &it->second;
}

void MemPool::push_root(Cons* a) { m_roots.push_back(a); }
void MemPool::pop_root() { m_roots.pop_back(); }

Cons* parse_expr(vcpkg::Parse::ParserBase& parser, MemPool& pool);

static void skip_whitespace(vcpkg::Parse::ParserBase& parser)
{
    parser.match_zero_or_more([](char32_t ch) { return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r'; });
}

static Cons* parse_list_tail(vcpkg::Parse::ParserBase& parser, MemPool& pool)
{
    if (parser.at_eof())
    {
        parser.add_error("unexpected eof");

        return nullptr;
    }

    if (parser.cur() == ')')
    {
        parser.next();
        return pool.intern_atom("nil");
    }
    auto e1 = parse_expr(parser, pool);
    if (!e1) return nullptr;

    pool.push_root(e1);

    skip_whitespace(parser);

    if (parser.cur() == '.')
    {
        parser.next();
        skip_whitespace(parser);
        auto e2 = parse_expr(parser, pool);
        pool.pop_root();
        if (!e2) return nullptr;
        skip_whitespace(parser);
        if (parser.cur() != ')')
        {
            return nullptr;
        }
        parser.next();
        return pool.alloc(e1, e2);
    }

    auto e2 = parse_list_tail(parser, pool);
    pool.pop_root();
    if (!e2) return nullptr;

    return pool.alloc(e1, e2);
}

static Cons* parse_expr(vcpkg::Parse::ParserBase& parser, MemPool& pool)
{
    if (parser.at_eof())
    {
        parser.add_error("unexpected eof");

        return nullptr;
    }

    if (parser.cur() == '(')
    {
        parser.next();
        skip_whitespace(parser);
        return parse_list_tail(parser, pool);
    }
    else if (parser.cur() == '.')
    {
        parser.add_error("unexpected '.' list concatenator");
        return nullptr;
    }
    else if (parser.cur() == '\'')
    {
        parser.next();
        auto inner_expr = parse_expr(parser, pool);
        if (!inner_expr) return nullptr;
        auto e2 = pool.alloc(inner_expr, pool.nil());
        pool.push_root(e2);
        auto quot = pool.intern_atom("quote");
        pool.pop_root();
        return pool.alloc(quot, e2);
    }
    else
    {
        auto sv = parser.match_until([](char32_t ch) { return ch == ')' || ch == ' '; });
        if (sv.size() == 0)
        {
            parser.add_error("expected expr");
            return nullptr;
        }
        return pool.intern_atom(sv.to_string());
    }
}

Cons* rlisp::parse(vcpkg::Parse::ParserBase& parser, MemPool& pool)
{
    skip_whitespace(parser);
    return parse_expr(parser, pool);
}

Cons* rlisp::parse(const char* data, const char* origin, MemPool& pool)
{
    vcpkg::Parse::ParserBase parser({data, strlen(data)}, {origin, strlen(origin)});
    auto p = parse(parser, pool);
    if (parser.get_error()) return nullptr;
    return p;
}

Cons* rlisp::parse(const char* data, MemPool& pool) { return parse(data, "", pool); }
