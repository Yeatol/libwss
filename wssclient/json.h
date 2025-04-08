#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

class json_value;

enum json_parse { STANDARD, COMMENTS };

class json final
{
public:
    enum json_type { NUL, NUMBER, BOOL, STRING, ARRAY, OBJECT };

    typedef std::vector<json> array;
    typedef std::map<std::string, json> object;

    json() noexcept;                // NUL
    json(std::nullptr_t) noexcept;  // NUL
    json(double value);             // NUMBER
    json(int value);                // NUMBER
    json(bool value);               // BOOL
    json(const std::string &value); // STRING
    json(std::string &&value);      // STRING
    json(const char * value);       // STRING
    json(const array &values);      // ARRAY
    json(array &&values);           // ARRAY
    json(const object &values);     // OBJECT
    json(object &&values);          // OBJECT

    template <class T, class = decltype(&T::to_json)>
    json(const T & t) : json(t.to_json()) {}

    template <class M, typename std::enable_if<std::is_constructible<std::string, decltype(std::declval<M>().begin()->first)>::value&& std::is_constructible<json, decltype(std::declval<M>().begin()->second)>::value,int>::type = 0>
    json(const M & m) : json(object(m.begin(), m.end())) {}

    template <class V, typename std::enable_if<std::is_constructible<json, decltype(*std::declval<V>().begin())>::value,int>::type = 0>
    json(const V & v) : json(array(v.begin(), v.end())) {}

    json(void *) = delete;

    json_type type() const;

    bool is_null()   const { return type() == NUL; }
    bool is_number() const { return type() == NUMBER; }
    bool is_bool()   const { return type() == BOOL; }
    bool is_string() const { return type() == STRING; }
    bool is_array()  const { return type() == ARRAY; }
    bool is_object() const { return type() == OBJECT; }

    double number_value() const;
    int int_value() const;
    bool bool_value() const;
    const std::string &string_value() const;
    const array &array_items() const;
    const object &object_items() const;
    const json & operator[](size_t i) const;
    const json & operator[](const std::string &key) const;

    void to_string(std::string &out) const;

    std::string to_string() const
    {
        std::string result;
        switch (type())
        {
        case json_type::NUMBER:
            result = double(int_value()) == number_value() ? std::to_string(int_value()) : std::to_string(number_value());
            break;
        case json_type::STRING:
            result = string_value();
            break;
        case json_type::NUL:
            break;
        default:
            to_string(result);
            break;
        }
        return result;
    }

    void erase(const std::string &key);

    void insert(const std::string &key, json value);

    static bool parse_error();

    static std::string& error_string();

    static void error_string(std::string error);

    static json parse(const std::string& in, json_parse strategy = json_parse::STANDARD);

    static json parse(const char* in, json_parse strategy = json_parse::STANDARD)
    {
        if (in)
        {
            return parse(std::string(in), strategy);
        }
        else
        {
            error_string("null input");
            return nullptr;
        }
    }
    static std::vector<json> parse_multi(const std::string & in, std::string::size_type & parser_stop_pos, json_parse strategy = json_parse::STANDARD);

    static inline std::vector<json> parse_multi(const std::string & in, json_parse strategy = json_parse::STANDARD)
    {
        std::string::size_type parser_stop_pos;
        return parse_multi(in, parser_stop_pos, strategy);
    }

    bool operator== (const json &rhs) const;
    bool operator<  (const json &rhs) const;
    bool operator!= (const json &rhs) const { return !(*this == rhs); }
    bool operator<= (const json &rhs) const { return !(rhs < *this);  }
    bool operator>  (const json &rhs) const { return  (rhs < *this);  }
    bool operator>= (const json &rhs) const { return !(*this < rhs);  }

    typedef std::initializer_list<std::pair<std::string, json_type>> shape;
    bool has_shape(const shape & types) const;

private:
    std::shared_ptr<json_value> m_ptr;
    static thread_local std::string shared_error;
};

class json_value
{
protected:
    friend class json;
    friend class json_int;
    friend class json_double;
    virtual json::json_type type() const = 0;
    virtual bool equals(const json_value * other) const = 0;
    virtual bool less(const json_value * other) const = 0;
    virtual void to_string(std::string &out) const = 0;
    virtual double number_value() const;
    virtual int int_value() const;
    virtual bool bool_value() const;
    virtual const std::string &string_value() const;
    virtual const json::array &array_items() const;
    virtual const json &operator[](size_t i) const;
    virtual const json::object &object_items() const;
    virtual const json &operator[](const std::string &key) const;
    virtual ~json_value() {}
public:
    virtual void erase(const std::string& key) = 0;
    virtual void insert(const std::string &key, json value) = 0;
};