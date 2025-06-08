#include <array>
#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::literals;

struct pan_t
{
    explicit pan_t(std::string_view v)
    {
        if (std::isalpha(v[0]))
        {
            scale_name_ = std::string(v);
            mass_ = 1;
        }
        else
        {
            int val;
            auto [p, ec] = std::from_chars(v.data(), v.data() + v.size(), val);
            if (p != v.data() + v.size() || val < 0)
                throw std::runtime_error("wrong mass value: "s + std::string(v));
            mass_ = val;
        }
    }

    const auto& scale_name() const noexcept
    {
        return scale_name_;
    }

    auto mass() const noexcept
    {
        return mass_;
    }

    auto add_mass() const noexcept
    {
        return add_mass_;
    }

    void add_mass(int v) noexcept
    {
        add_mass_ = v;
    }

private:
    std::string scale_name_;
    int mass_{};
    int add_mass_{};
};

struct scale_t
{
    scale_t(std::string_view left, std::string_view right)
        : pans_(pan_t(left), pan_t(right))
    {
    }

    const auto& left() const noexcept
    {
        return pans_.first;
    }

    const auto& right() const noexcept
    {
        return pans_.second;
    }

    auto& left() noexcept
    {
        return pans_.first;
    }

    auto& right() noexcept
    {
        return pans_.second;
    }

private:
    std::pair<pan_t, pan_t> pans_;
};

struct scale_tree_t
{
    void add(std::string_view name, std::string_view left, std::string_view right)
    {
        if (name.empty() || left.empty() || right.empty())
            throw std::runtime_error("empty scale name");

        for (const auto& s: {left, right})
        {
            if (std::isalpha(s[0]))
            {
                auto [it, inserted] = scale_refs_.emplace(s);
                if (!inserted)
                    throw std::runtime_error("scale is used more than once: "s + *it);
            }
        }
        auto [it, inserted] = tree_.try_emplace(std::string(name), left, right);
        if (!inserted)
            throw std::runtime_error("duplicate scale name: " + it->first);

        scale_names_.emplace_back(name);
    }

    void balance()
    {
        if (tree_root_.empty())
            find_tree_root();
        std::unordered_set<std::string> visited;
        do_balance(tree_root_, visited);
    }

    void print_add_mass(std::ostream& os) const
    {
        for (const auto& name: scale_names_)
        {
            auto it = tree_.find(name);
            if (it == std::end(tree_))
                throw std::runtime_error("scale not found: "s + name);
            const auto& scale = it->second;
            os << name << ',' << scale.left().add_mass() << ',' << scale.right().add_mass() << '\n';
        }
    }

private:
    void find_tree_root()
    {
        auto n = 0;
        for (const auto& scale_name: scale_names_)
        {
            if (!scale_refs_.contains(scale_name))
            {
                tree_root_ = scale_name;
                ++n;
            }
        }
        if (n == 0)
            throw std::runtime_error("cannot find tree root");
        if (n > 1)
            throw std::runtime_error("found tree roots: "s + std::to_string(n));
    }

    int do_balance(const std::string& scale_name, std::unordered_set<std::string>& visited)
    {
        if (!visited.emplace(scale_name).second)
            throw std::runtime_error("circular reference: "s + scale_name);
        auto it = tree_.find(scale_name);
        if (it != std::end(tree_))
        {
            auto& scale = it->second;

            const auto& left_scale_name = scale.left().scale_name();
            int left_mass = 0;
            if (!left_scale_name.empty())
                left_mass = do_balance(left_scale_name, visited);
            left_mass += scale.left().mass();

            const auto& right_scale_name = scale.right().scale_name();
            int right_mass = 0;
            if (!right_scale_name.empty())
                right_mass = do_balance(right_scale_name, visited);
            right_mass += scale.right().mass();

            int left_add_mass = 0;
            int right_add_mass = 0;
            if (left_mass > right_mass)
            {
                right_add_mass = left_mass - right_mass;
                scale.right().add_mass(right_add_mass);
            }
            else if (right_mass > left_mass)
            {
                left_add_mass = right_mass - left_mass;
                scale.left().add_mass(left_add_mass);
            }
            return left_mass + left_add_mass + right_mass + right_add_mass;
        }
        return 0;
    }

private:
    using tree_t = std::unordered_map<std::string, scale_t>;
    tree_t tree_;
    std::string tree_root_;
    std::vector<std::string> scale_names_;
    std::unordered_set<std::string> scale_refs_;
};

int main()
{
    scale_tree_t scales;
    std::string line;
    while (std::getline(std::cin, line))
    {
        line.erase(std::remove(std::begin(line), std::end(line), ' '), std::end(line));
        if (line.empty())
            continue;
        if (line[0] != '#')
        {
            //std::cout << line << '\n';
            auto begin = std::begin(line), it = begin;
            auto end = std::end(line);
            std::array<std::string_view, 3> tokens;
            size_t n = 0;
            for (; n < tokens.size(); ++it)
            {
                if (it == end || *it == ',')
                {
                    tokens[n++] = std::string_view(&*begin, std::distance(begin, it));
                    if (it == end)
                        break;
                    begin = std::next(it);
                }
            }
            if (n < 3 || it != end)
            {
                std::cerr << "wrong number of fields: " << line << '\n';
                std::exit(1);
            }
            scales.add(tokens[0], tokens[1], tokens[2]);
        }
    }
    scales.balance();
    scales.print_add_mass(std::cout);
}
