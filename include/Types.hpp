#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <variant>
#include <unordered_map>
#include <fstream>

class JsonData{
    public:
        enum class Type: uint8_t{
            Null,
            String,
            Number,
            Object,
            Array,
            Boolean
        };

        JsonData(Type type):m_type(type){}
        
        virtual std::string str(int indent) = 0;
        inline Type type() const { return m_type; }

    protected:
        Type m_type = Type::Null;
};

class JsonNull: public JsonData{
    public:
        JsonNull():JsonData(JsonData::Type::Null){}
        virtual std::string str(int indent) override { return "null"; }
};

class JsonString: public JsonData{
    public:
        JsonString():JsonData(JsonData::Type::String){}
        JsonString(const std::string& value):JsonData(JsonData::Type::String),data(value){}
        virtual std::string str(int indent) override { 
            //TODOO: format string before printing
            return std::string("\"")+data+std::string("\"");
        }

    private:
        std::string data;
};

class JsonNumber: public JsonData{
    public:
        JsonNumber():JsonData(JsonData::Type::Number){}
        JsonNumber(int value):JsonData(JsonData::Type::Number),data(value){}
        JsonNumber(float value):JsonData(JsonData::Type::Number),data(value){}

        virtual std::string str(int indent) override { return isInteger()?std::to_string(std::get<int>(data)):std::to_string(std::get<float>(data)); }
        inline bool isInteger() const { return std::holds_alternative<int>(data);}
        inline bool isFloat() const { return std::holds_alternative<float>(data);}
        inline float asFloat() const { return isInteger()?float(std::get<int>(data)):std::get<float>(data); }
        inline float asInteger() const { return isFloat()?int(std::get<float>(data)):std::get<int>(data); }

    private:
        std::variant<float,int32_t> data = 0.0f;
};

class JsonArray: public JsonData{
    public:
        JsonArray():JsonData(JsonData::Type::Array){}
        JsonArray(const std::vector<std::shared_ptr<JsonData>>& values):JsonData(JsonData::Type::Array),m_data(values.begin(),values.end()){
            printf("json array created\n");
            for(auto &value:values){
                printf("value: %s\n",value->str(0).c_str());
            }
        }

        void push_back(const std::shared_ptr<JsonData>& item){ m_data.emplace_back(std::move(item)); }
        void pop_back(){ m_data.pop_back(); }
        inline size_t size() const { return m_data.size(); }
        inline const std::vector<std::shared_ptr<JsonData>> data() const { return m_data; }

        virtual std::string str(int indent) override { 
            std::stringstream ss;
            ss<<"[";
            for(auto i=m_data.begin();i != m_data.end();i++){
                ss<<i->get()->str(indent+1);
                if(i != m_data.end()-1 ) ss<<", ";
            }
            ss<<"]";
            return ss.str();
        }

    private:
        std::vector<std::shared_ptr<JsonData>> m_data;
};

class JsonObject: public JsonData{
    public:
        using ObjectList = std::vector<std::pair<std::string,std::shared_ptr<JsonData>>>;

        JsonObject():JsonData(JsonData::Type::Object){}
        JsonObject(const ObjectList& values):JsonData(JsonData::Type::Object),m_data(values.begin(),values.end()){}

        void insert(const std::string& key,const std::shared_ptr<JsonData>& value){ m_data.insert_or_assign(key,value); }
        void erase(const std::string& key){ m_data.erase(key); }

        inline size_t size() const { return m_data.size(); }
        inline const std::unordered_map<std::string,std::shared_ptr<JsonData>> data() const { return m_data; }

        virtual std::string str(int indent) { 
            std::stringstream ss;
            ss<<"{\n";
            for(auto i=m_data.begin();i != m_data.end();){
                for(auto i=0;i<indent;i++)ss<<"\t";
                ss<<"\t\""<<i->first<<"\":"<<i->second->str(indent+1);
                if( ++i != m_data.end() ) ss<<",";
                ss<<"\n";
            }
            for(auto i=0;i<indent;i++)ss<<"\t";
            ss<<"}";
            return ss.str();
        }

    private:
        std::unordered_map<std::string,std::shared_ptr<JsonData>> m_data;
};

struct JsonValue{
    std::shared_ptr<JsonData> sp_data = nullptr;

    using ValueList = std::vector<JsonValue>;
    using ObjectList = std::vector<std::pair<std::string,JsonValue>>;

    JsonValue(){ sp_data.reset(new JsonNull()); }
    JsonValue(void*){ sp_data.reset(new JsonNull()); }
    JsonValue(int value){ sp_data.reset(new JsonNumber(value)); }
    JsonValue(float value){ sp_data.reset(new JsonNumber(value)); }
    JsonValue(const std::string& value){ sp_data.reset(new JsonString(value)); }
    JsonValue(const ValueList& values){
        std::vector<std::shared_ptr<JsonData>> _values;
        _values.reserve(values.size());
        for(auto &value:values){
            _values.emplace_back(std::move(value.sp_data));
        }
        sp_data.reset(new JsonArray(_values));
    }
    JsonValue(const ObjectList& values){
        std::vector<std::pair<std::string,std::shared_ptr<JsonData>>> _values;
        _values.reserve(values.size());
        for(auto &value:values){
            std::pair<std::string,std::shared_ptr<JsonData>> _data{value.first, std::move(value.second.sp_data) };
            _values.emplace_back(std::move(_data));
        }
        sp_data.reset(new JsonObject(_values));
    }

    inline bool valid() const { return sp_data!=nullptr; }
    inline JsonData::Type type() const { return sp_data->type(); }
    inline std::string str(int indent=0) const { return sp_data->str(indent); }
};

class JsonDocument{
    public:
        using ValueList = std::vector<JsonValue>;
        using ObjectList = std::vector<std::pair<std::string,JsonValue>>;
        using SValueList = std::vector<std::shared_ptr<JsonData>>;
        using SObjectList = std::vector<std::pair<std::string,std::shared_ptr<JsonData>>>;

        JsonDocument(){}
        JsonDocument(const ValueList& values){
            printf("crating json array\n");
            std::vector<std::shared_ptr<JsonData>> _values;
            _values.reserve(values.size());
            for(auto &value:values) _values.emplace_back(std::move(value.sp_data));
            p_data.reset(new JsonArray(_values));
        }
        JsonDocument(const ObjectList& values){
            std::vector<std::pair<std::string,std::shared_ptr<JsonData>>> _values;
            _values.reserve(values.size());
            for(auto &value:values){
                std::pair<std::string,std::shared_ptr<JsonData>> _data{value.first,std::move(value.second.sp_data) };
                _values.emplace_back(std::move(_data));
            }
            p_data.reset(new JsonObject(_values));
        }
        JsonDocument(const SValueList& values){
            p_data.reset(new JsonArray(values));
        }
        JsonDocument(const SObjectList& values){
            p_data.reset(new JsonObject(values));
        }

        inline bool valid() const { return p_data!=nullptr; }

        friend std::ostream& operator<<(std::ostream& os, const JsonDocument& document);

    protected:
        std::unique_ptr<JsonData> p_data = nullptr;
};

std::ostream& operator<<(std::ostream& os, const JsonDocument& document){
    os<<document.p_data->str(0)<<std::endl;
    return os;
}
