
#ifndef _C4_RYML_HPP_
#include <ryml/ryml.hpp>
#endif

#include <ctype.h>

namespace c4 {
namespace yml {

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

size_t Node::id() const
{
    return m_s->id(this);
}

Node * Node::prev_node() const
{
    return m_s->get(m_list.prev);
}

Node * Node::next_node() const
{
    return m_s->get(m_list.next);
}

Node * Node::parent() const
{
    return m_s->get(m_parent);
}

size_t Node::num_children() const
{
    if(is_val()) return 0;
    size_t count = 0;
    for(Node *n = m_s->get(m_children.first); n; n = n->next_sibling())
    {
        ++count;
    }
    return count;
}

Node * Node::child(size_t i) const
{
    if(is_val()) return nullptr;
    size_t count = 0;
    for(Node *n = m_s->get(m_children.first); n; n = n->next_sibling(), ++count)
    {
        if(count == i) return n;
    }
    C4_ASSERT(i < count);
    return nullptr;
}
Node * Node::first_child() const
{
    if(is_val()) return nullptr;
    return m_s->get(m_children.first);
}
Node * Node::last_child() const
{
    if(is_val()) return nullptr;
    return m_s->get(m_children.last);
}

Node * Node::find_child(cspan const& name) const
{
    if(is_val()) return nullptr;
    C4_ASSERT(m_type == TYPE_DOC || m_type == TYPE_MAP);
    C4_ASSERT(bool(name) == true);
    if(m_children.first == NONE)
    {
        C4_ASSERT(m_children.last == NONE);
        return nullptr;
    }
    else
    {
        C4_ASSERT(m_children.last != NONE);
    }
    for(Node *n = m_s->get(m_children.first); n; n = n->next_sibling())
    {
        if(n->m_name == name)
        {
            return n;
        }
    }
    return nullptr;
}

bool Node::is_child(Node const* ch) const
{
    for(Node const* n = first_child(); n; n = n->next_sibling())
    {
        if(n == ch) return true;
    }
    return false;
}

size_t Node::num_siblings() const
{
    return (m_parent != NONE) ? m_s->get(m_parent)->num_children() : 0;
}

Node * Node::sibling(size_t i) const
{
    C4_ASSERT(parent() != nullptr);
    return m_s->get(m_parent)->child(i);
}

Node * Node::first_sibling() const
{
    return m_s->get(m_parent)->first_child();
}

Node * Node::last_sibling() const
{
    return m_s->get(m_parent)->last_child();
}

Node * Node::find_sibling(cspan const& name) const
{
    return m_s->get(m_parent)->find_child(name);
}

Node * Node::prev_sibling() const
{
    if(m_siblings.prev == NONE) return nullptr;
    Node *n = m_s->get(m_siblings.prev);
    C4_ASSERT(!n || n->m_parent == m_parent);
    return n;
}

Node * Node::next_sibling() const
{
    if(m_siblings.next == NONE) return nullptr;
    Node *n = m_s->get(m_siblings.next);
    C4_ASSERT(!n || n->m_parent == m_parent);
    return n;
}

bool Node::is_sibling(Node const* s) const
{
    for(Node *n = first_sibling(); n; n = n->next_sibling())
    {
        if(n == s) return true;
    }
    return false;
}

Node * Node::insert_sibling(cspan const& name, cspan const& val, Node * after)
{
    C4_ASSERT( ! after || (is_sibling(after) && after->is_sibling(this)));
    C4_ASSERT(m_s->get(m_parent)->is_container());
    m_s->_stack_push(m_s->get(m_parent));
    Node *n = m_s->add_val(name, val, after);
    m_s->_stack_pop();
    return n;
}

Node * Node::insert_sibling(cspan const& name, NodeType_e sibtype, Node * after)
{
    C4_ASSERT( ! after || (is_sibling(after) && after->is_sibling(this)));
    C4_ASSERT(m_s->get(m_parent)->is_container());
    Node *n = _insert_by_type(m_s->get(m_parent), name, sibtype, after);
    return n;
}

Node * Node::insert_sibling(Node *sib, Node * after)
{
    C4_ASSERT(sib && ( ! is_sibling(sib) && ! sib->is_sibling(this)));
    C4_ASSERT( ! after || (is_sibling(after) && after->is_sibling(this)));
    C4_ASSERT( ! sib->name().empty() == is_map());
    C4_ASSERT(   sib->name().empty() == is_seq());
    C4_ASSERT( ! sib->val().empty() == (sib->m_type == TYPE_VAL));
    C4_ASSERT(   sib->val().empty() == (sib->is_container()));
    m_s->set_parent(m_s->get(m_parent), sib, after);
    return sib;
}


Node * Node::insert_child(cspan const& name, cspan const& val, Node * after)
{
    C4_ASSERT( ! after || is_child(after));
    C4_ASSERT(is_container());
    m_s->_stack_push(this);
    m_s->add_val(name, val, after);
    m_s->_stack_pop();
    return nullptr;
}

Node * Node::insert_child(cspan const& name, NodeType_e chtype, Node * after)
{
    C4_ASSERT( ! after || is_child(after));
    Node *n = _insert_by_type(this, name, chtype, after);
    return n;
}

Node * Node::insert_child(Node *ch, Node * after)
{
    C4_ASSERT( ! is_child(ch));
    C4_ASSERT( ! after || is_child(after));
    m_s->set_parent(this, ch, after);
    return ch;
}


Node * Node::_insert_by_type(Node *which_parent, cspan const& name, NodeType_e type, Node *after)
{
    m_s->_stack_push(which_parent);
    Node *n = nullptr;
    switch(type)
    {
    case TYPE_VAL:
        n = m_s->add_val(name, {}, after);
        break;
    case TYPE_SEQ:
        n = m_s->begin_seq(name, after);
        m_s->end_seq();
        break;
    case TYPE_MAP:
        n = m_s->begin_map(name, after);
        m_s->end_map();
        break;
    case TYPE_DOC:
        n = m_s->begin_doc(after);
        m_s->end_doc();
        break;
    case TYPE_NONE:
        RymlCallbacks::error("cannot add sibling when the current node type is NONE");
        break;
    case TYPE_ROOT:
        RymlCallbacks::error("cannot add sibling when the current node type is ROOT");
        break;
    default:
        RymlCallbacks::error("unknown node type to add as sibling");
        break;
    }
    m_s->_stack_pop();
    return n;
}

Node * Node::remove_sibling(cspan const& name)
{
    C4_ASSERT(find_sibling(name));
    Node *n = find_sibling(name);
    return remove_sibling(n);
}

Node * Node::remove_sibling(size_t i)
{
    C4_ASSERT(sibling(i));
    Node *n = sibling(i);
    return remove_sibling(n);
}

Node * Node::remove_sibling(Node *sib)
{
    C4_ASSERT(sib && ( ! is_sibling(sib) && ! sib->is_sibling(this)));
    C4_ASSERT(false && "not implemented");
    return sib;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

Tree::Tree()
    :
    m_buf(nullptr),
    m_num(0),
    m_head(NONE),
    m_tail(NONE),
    m_free_head(NONE),
    m_free_tail(NONE),
    m_load_root_id(NONE)
{
}

Tree::Tree(size_t sz) : Tree()
{
    reserve(sz);
}

Tree::~Tree()
{
    if(m_buf)
    {
        RymlCallbacks::free(m_buf, m_num * sizeof(Node));
    }
}

//-----------------------------------------------------------------------------
void Tree::serialize(Node * root, span const* buffer) const
{
    C4_ASSERT(false && "not implemented");
}

Node * Tree::load(Node * root, cspan const& yml_str, Parser *p_)
{
    Parser p, *ptr;
    ptr = p_ ? p_ : &p;

    C4_ASSERT(root->is_map());

    size_t idr = root->id();

    set_load_root(root);
    ptr->set_load_root(root);
    ptr->parse(this, yml_str);
    ptr->set_load_root(nullptr);
    set_load_root(nullptr);

    return get(idr);
}

//-----------------------------------------------------------------------------
void Tree::reserve(size_t sz)
{
    if(sz <= m_num) return;
    Node *buf = (Node*)RymlCallbacks::allocate(sz * sizeof(Node), nullptr);
    if(m_buf)
    {
        memcpy(buf, m_buf, m_num * sizeof(Node));
        RymlCallbacks::free(m_buf, m_num * sizeof(Node));
    }
    if(m_free_head == NONE)
    {
        C4_ASSERT(m_free_tail == m_free_head);
        m_free_head = m_num;
        m_free_tail = sz;
    }
    else
    {
        C4_ASSERT(m_free_tail != NONE);
        m_buf[m_free_tail].m_list.next = m_num;
    }
    size_t first = m_num, del = sz - m_num;
    m_num = sz;
    m_buf = buf;
    clear_range(first, del);
}

//-----------------------------------------------------------------------------
void Tree::clear()
{
    clear_range(0, m_num);
    m_head = NONE;
    m_tail = NONE;
    m_free_head = 0;
    m_free_tail = m_num;
}

//-----------------------------------------------------------------------------
void Tree::clear_range(size_t first, size_t num)
{
    if(num == 0) return; // prevent overflow when subtracting
    C4_ASSERT(first >= 0 && first + num <= m_num);
    memset(m_buf + first, 0, num * sizeof(Node));
    for(size_t i = first, e = first + num; i < e; ++i)
    {
        Node *n = m_buf + i;
        n->m_list.prev = i - 1;
        n->m_list.next = i + 1;
        n->m_parent = NONE;
        n->m_children.first = NONE;
        n->m_children.last = NONE;
        n->m_siblings.prev = NONE;
        n->m_siblings.next = NONE;
    }
    m_buf[first + num - 1].m_list.next = NONE;
}

//-----------------------------------------------------------------------------
Node *Tree::claim(Node *after)
{
    C4_ASSERT(after == nullptr || (m_buf <= after && (after <= m_buf + m_num)));
    size_t last = after ? after - m_buf : m_tail;
    Node *n = claim(last, NONE);
    return n;
}

//-----------------------------------------------------------------------------
Node *Tree::claim(size_t prev, size_t next)
{
    C4_ASSERT(prev == NONE || prev >= 0 && prev < m_num);
    C4_ASSERT(next == NONE || next >= 0 && next < m_num);
    if(m_free_head == NONE || m_buf == nullptr)
    {
        size_t sz = 2 * m_num;
        sz = sz ? sz : 16;
        reserve(sz);
    }
    size_t f = m_free_head;
    Node *n = m_buf + f;
    m_free_head = n->m_list.next;
    if(m_free_head == NONE)
    {
        m_free_tail = NONE;
    }
    n->m_s = this;
    n->m_list.prev = prev;
    n->m_list.next = next;
    n->m_siblings.prev = NONE;
    n->m_siblings.next = NONE;
    n->m_children.first = NONE;
    n->m_children.last = NONE;
    if(prev == NONE) m_head = f;
    else
    {
        Node *p = m_buf + prev;
        p->m_list.next = f;
    }
    if(next == NONE) m_tail = f;
    else
    {
        Node *v = m_buf + next;
        v->m_list.prev = f;
    }
    return n;
}

//-----------------------------------------------------------------------------
void Tree::set_parent(Node *parent, Node *child, Node *prev_sibling, Node *next_sibling)
{
    C4_ASSERT(child != nullptr && child >= m_buf && child < m_buf + m_num);
    C4_ASSERT(parent == nullptr || parent >= m_buf && parent < m_buf + m_num);

    child->m_parent = parent ? parent - m_buf : NONE;

    if( ! prev_sibling)
    {
        next_sibling = parent->first_child();
    }

    if( ! next_sibling)
    {
        if(prev_sibling)
        {
            next_sibling = prev_sibling->next_sibling();
        }
    }

    child->m_siblings.prev = NONE;
    child->m_siblings.next = NONE;
    if(prev_sibling)
    {
        C4_ASSERT(prev_sibling->next_sibling() == next_sibling);
        child->m_siblings.prev = prev_sibling->id();
        prev_sibling->m_siblings.next = child->id();
    }
    if(next_sibling)
    {
        C4_ASSERT(next_sibling->prev_sibling() == prev_sibling);
        child->m_siblings.next = next_sibling->id();
        next_sibling->m_siblings.prev = child->id();
    }

    if( ! parent) return;
    if(parent->m_children.first == NONE)
    {
        C4_ASSERT(parent->m_children.last == NONE);
        parent->m_children.first = child->id();
        parent->m_children.last = child->id();
    }
    else
    {
        if(child->m_siblings.next == parent->m_children.first)
        {
            parent->m_children.first = child->id();
        }
        if(child->m_siblings.prev == parent->m_children.last)
        {
            parent->m_children.last = child->id();
        }
    }
}

//-----------------------------------------------------------------------------
void Tree::free(size_t i)
{
    C4_ASSERT(i >= 0 && i < m_num);
    m_buf[i].m_list.next = m_free_head;
    m_buf[i].m_list.prev = NONE;
    m_buf[m_free_head].m_list.prev = i;
    m_free_head = i;
    if(m_free_tail == NONE)
    {
        m_free_tail = m_free_head;
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void NextParser::parse(const char *file, cspan const& buf, Node *root)
{
    m_file = file ? file : "(no file)";
    m_buf = buf;
    m_state.reset(file, root);

    while(m_state.pos.offset < m_buf.len)
    {
        m_state.line_scanned(_scan_line());

        while(m_state.line_contents.ltrim)
        {
            _handle_line(m_state.line_contents.ltrim);
        }
    }
}

//-----------------------------------------------------------------------------
void NextParser::_handle_line(cspan ltrim)
{
    C4_ASSERT( ! ltrim.empty());

    if(m_state.has_any(RMAP|RSEQ|BLCK))
    {
        if(_indentation_popped())
        {
            printf("indentation popped!\n");
            if(m_state.has_any(RMAP))
            {
                _pop_map(ltrim);
            }
            else if(m_state.has_any(RSEQ))
            {
                _pop_seq(ltrim);
            }
            else if(m_state.has_any(BLCK))
            {
                RymlCallbacks::error("not implemented");
            }
            return;
        }
    }

    if(m_state.has_any(RKEY|RVAL|RUNK|RTOP))
    {
        const bool alnum = isalnum(ltrim[0]);
        const bool dquot = ltrim.begins_with('"');
        const bool squot = ltrim.begins_with('\'');
        const bool block = ltrim.begins_with('|') || ltrim.begins_with('>');

        if(alnum || dquot || squot || block)
        {
            // it's a scalar
            printf("it's a scalar\n");
            cspan scalar = _read_scalar(ltrim, dquot || squot);
            C4_ASSERT( ! scalar.empty());
            printf("scalar was %.*s\n", _c4prsp(scalar));
            if(m_state.has_any(RKEY|RVAL))
            {
                C4_ASSERT(m_state.has_any(RKEY) != m_state.has_any(RVAL));
                C4_ASSERT(m_state.has_any(RKEY) != m_state.has_any(SSCL));
            }
            else if( ! m_state.has_any(RTOP|RUNK))
            {
                RymlCallbacks::error("was not expecting a scalar");
            }

            if(m_state.has_any(RUNK))
            {
                if(m_state.has_none(SSCL))
                {
                    _store_scalar(scalar);
                }
                if(m_state.has_any(SSCL))
                {
                    _append_key_val(m_state.scalar, scalar);
                    m_state.flags &= ~SSCL;
                }
            }
            else if(m_state.has_all(RKEY|RMAP))
            {
                _store_scalar(scalar);
                m_state.flags &= ~RKEY;
                m_state.flags |= RVAL;
            }
            else if(m_state.has_any(RSEQ))
            {
                _store_scalar(scalar);
                m_state.flags &= ~RKEY;
                m_state.flags |= RVAL;
            }
            else
            {
                RymlCallbacks::error("wtf?");
            }
            return;
        }
    }

    size_t ind = m_state.line_contents.indentation;
    if(m_state.flags & RUNK)
    {
        if(ltrim.begins_with("- ") || ltrim.begins_with('['))
        {
            // start a sequence
            printf("start a sequence\n");
            size_t nextstate = RSEQ|RVAL;
            if(ltrim.begins_with('['))
            {
                nextstate |= EXPL;
                m_state.line_progressed(1);
            }
            else if(ltrim.begins_with("- "))
            {
                ltrim = ltrim.subspan(2);
                m_state.line_progressed(2);
            }
            _push_seq(ltrim, nextstate, false);
            return;
        }
        else if(ltrim.begins_with('{') || ltrim.begins_with("? "))
        {
            // start a mapping
            printf("start a mapping\n");
            cspan k = ltrim.subspan(2);
            printf("...key@%zd: %.*s\n", ind, _c4prsp(k));
            size_t nextstate = RMAP|RKEY;
            if(ltrim.begins_with('{'))
            {
                nextstate |= EXPL;
            }
            else if(ltrim.begins_with("? "))
            {
                ltrim = ltrim.subspan(2);
            }
            _push_map(ltrim, nextstate, false);
            return;
        }
        else if(m_state.has_any(SSCL))
        {
            if(ltrim.begins_with(','))
            {
                _push_seq(ltrim, RSEQ, false);
                m_state.line_progressed(1);
            }
            else if(ltrim.begins_with(": "))
            {
                _push_map(ltrim, RMAP, false);
                m_state.line_progressed(1);
            }
        }
    }

    if(ltrim.begins_with('#'))
    {
        // a comment line
        printf("a comment line\n");
        m_state.line_progressed(ltrim.len);
        return;
    }
    else if(ltrim.begins_with('!'))
    {
        printf("it's a tag\n");
        RymlCallbacks::error("tags not implemented");
    }
    else if(ltrim.begins_with('|'))
    {
        // a block start line
        printf("a block start line\n");
        size_t next_state = BLCK|KEEP;
        if(ltrim.begins_with("|-"))
        {
            next_state = BLCK|STRP;
        }
    }
    else if(ltrim.begins_with('>'))
    {
        // a folded block start line
        printf("a folded block start line\n");
        RymlCallbacks::error("folded blocks not implemented");
    }
    else
    {
        // it's probably a scalar
        if(ltrim.begins_with('"') || ltrim.begins_with('\''))
        {
            printf("it's a scalar\n");
        }

        size_t first_colon = ltrim.first_of(':');
        if(first_colon != npos && first_colon > 0)
        {
            // a simple key - start a mapping
            printf("a simple key - start a mapping");
            cspan k = ltrim.subspan(0, first_colon);
            printf("...key@%zd: %.*s\n", ind, _c4prsp(k));
            size_t nextstate = RMAP;
            _push_map(ltrim, nextstate, (m_state.flags & RUNK));
        }
        // must be a scalar
        printf("must be a scalar\n");
    }

    // the following tokens can appear only at top level
    if(m_state.flags & RTOP)
    {
        if(ltrim.begins_with('%'))
        {
            printf("%% directive!\n");
            if(ltrim.begins_with("%YAML"))
            {
            }
            else if(ltrim.begins_with("%TAG"))
            {
            }
            else
            {
                RymlCallbacks::error("unknown directive starting with %");
            }
            RymlCallbacks::error("not implemented");
        }
        else if((m_state.flags & RTOP) && ltrim.begins_with("---"))
        {
            C4_ASSERT(m_state.level == 0);
            // end/start a document
            printf("end/start a document\n");
        }
        else if((m_state.flags & RTOP) && ltrim.begins_with("..."))
        {
            // end a document
            printf("end a document\n");
        }
    }
}

//-----------------------------------------------------------------------------
cspan NextParser::_read_scalar(cspan const& line_remainder, bool known_quoted)
{
    cspan s = line_remainder;

    if( ! known_quoted)
    {
        auto pos = s.first_of("'\"");
        if(pos != npos)
        {
            RymlCallbacks::error("reading quoted scalars not yet implemented (hint: there can be colons inside the quotes)");
        }
    }

    size_t colon_space = s.find(": ");
    if(m_state.has_any(RKEY))
    {
        if(m_state.has_any(CPLX))
        {
            C4_ASSERT(m_state.has_any(RMAP));
            s = s.left_of(colon_space);
        }
        else
        {
            s = s.triml(' ');
            s = s.left_of(colon_space);
            s = s.trimr(' ');
        }

        if(colon_space != npos)
        {
            colon_space += 2;
        }
        else
        {
            colon_space = s.len + (line_remainder.end() - s.end());
        }
        m_state.line_progressed(colon_space);
    }
    else if(m_state.has_any(RVAL))
    {
        if(m_state.has_any(BLCK))
        {
            RymlCallbacks::error("not implemented");
        }
        s = s.trim(' ');
        size_t last = s.len + (line_remainder.end() - s.end());
        m_state.line_progressed(last);
    }
    else
    {
        s = s.left_of(s.last_not_of(",: "));
        size_t last = s.len + (line_remainder.end() - s.end());
        m_state.line_progressed(last);
    }

    return s;
}

//-----------------------------------------------------------------------------
cspan NextParser::_scan_line()
{
    if(m_state.pos.offset >= m_buf.len) return {};
    char const* b = &m_buf[m_state.pos.offset];
    char const* e = b;
    while( ! _matches(*e, "\r\n\0"))
    {
        ++e;
    }
    size_t len = e - b;
    cspan ret = m_buf.subspan(m_state.pos.offset, len);
    // advance pos, including line ending chars
    while(_matches(*e, "\r\n\0"))
    {
        ++e;
        ++len;
    }
    m_state.line_ended(len);
    return ret;
}

} // namespace ryml
} // namespace c4