#pragma once
#include <string>
#include <map>
#include <fstream>
#include <memory>
#include "../core/error.hpp"

namespace xebec {

class TemplateEngine {
public:
    virtual ~TemplateEngine() = default;
    
    void set_template_dir(const std::string& dir) {
        template_dir_ = dir;
    }
    
    std::string render(const std::string& template_name, 
                      const std::map<std::string, std::string>& vars) {
        std::string template_content = load_template(template_name);
        return render_template(template_content, vars);
    }

protected:
    virtual std::string render_template(const std::string& content,
                                      const std::map<std::string, std::string>& vars) = 0;

private:
    std::string template_dir_;
    
    std::string load_template(const std::string& name) {
        std::ifstream file(template_dir_ + "/" + name);
        if (!file.is_open()) {
            throw HttpError(500, "Template not found: " + name);
        }
        return std::string((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    }
};

class SimpleTemplateEngine : public TemplateEngine {
protected:
    std::string render_template(const std::string& content,
                              const std::map<std::string, std::string>& vars) override {
        std::string result = content;
        for (const auto& [key, value] : vars) {
            std::string placeholder = "{{" + key + "}}";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), value);
                pos += value.length();
            }
        }
        return result;
    }
};

} // namespace xebec 