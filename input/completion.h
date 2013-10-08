
#ifndef _COMPLETION_H_
#define _COMPLETION_H_

#include <vector>
#include <string>
#include <stdexcept>

namespace input {

/** String completion class. */
class Completion
{
public:
    /** Constructor. */
    Completion();

    /** Add a item to complete.
     * @param item item
     * @param cache If true, item is added to cached items if it matches the prefix without need to call get_matches(). */
    Completion& add_item(const std::string &item, bool cache=true);

    /** Add items to complete. */
    Completion& add_items(const std::vector<std::string> &items);

    /** Remove all items and clear the cache. */
    Completion& clear() { m_items.clear(); m_cachedItems.clear(); return *this; }

    /** Call before calling next() or get_matches() */
    Completion& create_cache();

    /** Returns all strings which matches the prefix. */
    std::vector<std::string> get_matches() { return m_cachedItems; }

    /** Returns the next string matching the prefix.
     * @throw std::out_of_range if any of the items doesn't match the prefix. */
    std::string next() throw(std::out_of_range);

    bool has_next() const { return m_cachedItems.size() != 0; }

    Completion& set_prefix(const std::string &prefix) { m_prefix = prefix; return *this; }

    typedef bool (*Comparator)(std::string, std::string);

    /** Set comparator so that you can treat '[FIN]morning' as 'morning' */
    void set_comparator(Comparator comparator) { m_comparator = comparator; }
private:
    Comparator m_comparator;
    std::vector<std::string> m_items;
    std::vector<std::string> m_cachedItems;
    int m_currentItem;
    std::string m_prefix;
};

} // namespace input

#endif // _COMPLETION_H_
