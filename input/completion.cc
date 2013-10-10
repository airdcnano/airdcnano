#include <algorithm>
#include "completion.h"

namespace input {

bool _default_comparator(std::string s1, std::string s2)
{
    return s1.find(s2) == 0;
}

Completion::Completion():
    m_comparator(_default_comparator),
    m_items(),
    m_cachedItems(),
    m_currentItem(-1),
    m_prefix()
{

}

Completion& Completion::add_item(const std::string &item, bool cache)
{
    m_items.push_back(item);
    std::sort(m_cachedItems.begin(), m_cachedItems.end());
    if(cache && m_comparator(item, m_prefix)) {
        m_cachedItems.push_back(item);
        std::sort(m_cachedItems.begin(), m_cachedItems.end());
    }
    return *this;
}

Completion& Completion::add_items(const std::vector<std::string> &items)
{
    std::copy(items.begin(), items.end(), std::back_inserter(m_items));
    return *this;
}

Completion& Completion::create_cache()
{
    m_cachedItems.clear();
    // since there is not std::copy_if
	//std::copy_if(m_items.begin(), m_items.end(), );

    std::remove_copy_if(m_items.begin(), m_items.end(),
            std::back_inserter(m_cachedItems),
            std::not1(std::bind2nd(
                    std::ptr_fun(m_comparator), m_prefix)));
    std::sort(m_cachedItems.begin(), m_cachedItems.end());
    return *this;
}

std::string Completion::next()
    throw(std::out_of_range)
{
    if(m_cachedItems.size() == 0)
        throw std::out_of_range("Completion::next()");

    if(++m_currentItem > static_cast<int>(m_cachedItems.size()-1) || m_currentItem == -1)
        m_currentItem = 0;

    return m_cachedItems.at(m_currentItem);
}

} // namespace input
