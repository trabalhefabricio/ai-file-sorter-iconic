#include <catch2/catch_test_macros.hpp>
#include "InputValidator.hpp"

#include <filesystem>
#include <fstream>

using namespace afs;

TEST_CASE("InputValidator - validate_non_empty", "[validator]") {
    SECTION("Empty string fails") {
        auto result = InputValidator::validate_non_empty("", "test_field");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::EmptyInput);
        REQUIRE(result.error().message.find("test_field") != std::string::npos);
    }
    
    SECTION("Whitespace-only fails") {
        auto result = InputValidator::validate_non_empty("   \t\n", "test_field");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::EmptyInput);
    }
    
    SECTION("Non-empty string succeeds") {
        auto result = InputValidator::validate_non_empty("hello", "test_field");
        
        REQUIRE(result.is_ok());
    }
    
    SECTION("String with spaces is valid") {
        auto result = InputValidator::validate_non_empty("hello world", "test_field");
        
        REQUIRE(result.is_ok());
    }
}

TEST_CASE("InputValidator - validate_directory_path", "[validator]") {
    SECTION("Empty path fails") {
        auto result = InputValidator::validate_directory_path("");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::EmptyInput);
    }
    
    SECTION("Non-existent path fails when must_exist=true") {
        auto result = InputValidator::validate_directory_path(
            "/nonexistent/path/that/does/not/exist", true);
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::PathNotFound);
    }
    
    SECTION("Non-existent path succeeds when must_exist=false") {
        auto result = InputValidator::validate_directory_path(
            "/nonexistent/path", false);
        
        REQUIRE(result.is_ok());
    }
    
    SECTION("Existing directory succeeds") {
        // Use current directory which should always exist
        auto result = InputValidator::validate_directory_path(".", true);
        
        REQUIRE(result.is_ok());
    }
}

TEST_CASE("InputValidator - validate_api_key", "[validator]") {
    SECTION("Empty key fails") {
        auto result = InputValidator::validate_api_key("", "TestProvider");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::EmptyInput);
        REQUIRE(result.error().message.find("TestProvider") != std::string::npos);
    }
    
    SECTION("Whitespace-only key fails") {
        auto result = InputValidator::validate_api_key("   ", "TestProvider");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::InvalidApiKey);
    }
    
    SECTION("Short key fails") {
        auto result = InputValidator::validate_api_key("short", "TestProvider");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::InvalidApiKey);
    }
    
    SECTION("Placeholder key fails") {
        auto result = InputValidator::validate_api_key(
            "your-api-key-goes-here-12345", "TestProvider");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::InvalidApiKey);
        REQUIRE(result.error().message.find("placeholder") != std::string::npos);
    }
    
    SECTION("Valid-looking key succeeds") {
        auto result = InputValidator::validate_api_key(
            "sk-abcdefghij1234567890abcdefghij", "OpenAI");
        
        REQUIRE(result.is_ok());
    }
}

TEST_CASE("InputValidator - validate_category_label", "[validator]") {
    SECTION("Empty label fails") {
        auto result = InputValidator::validate_category_label("", "category");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::EmptyInput);
    }
    
    SECTION("Label with leading space fails") {
        auto result = InputValidator::validate_category_label(" Documents", "category");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::InvalidInput);
    }
    
    SECTION("Label with trailing space fails") {
        auto result = InputValidator::validate_category_label("Documents ", "category");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::InvalidInput);
    }
    
    SECTION("Valid label succeeds") {
        auto result = InputValidator::validate_category_label("Documents", "category");
        
        REQUIRE(result.is_ok());
    }
    
    SECTION("Label with internal spaces succeeds") {
        auto result = InputValidator::validate_category_label(
            "My Documents", "category");
        
        REQUIRE(result.is_ok());
    }
    
    SECTION("Reserved name fails") {
        auto result = InputValidator::validate_category_label("CON", "category");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::InvalidInput);
        REQUIRE(result.error().message.find("reserved") != std::string::npos);
    }
    
    SECTION("Label with invalid characters fails") {
        auto result = InputValidator::validate_category_label(
            "Documents<>", "category");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::InvalidInput);
        REQUIRE(result.error().message.find("invalid characters") != std::string::npos);
    }
}

TEST_CASE("InputValidator - validate_model_name", "[validator]") {
    SECTION("Empty model name fails") {
        auto result = InputValidator::validate_model_name("");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::EmptyInput);
    }
    
    SECTION("Valid model names succeed") {
        REQUIRE(InputValidator::validate_model_name("gpt-4").is_ok());
        REQUIRE(InputValidator::validate_model_name("gpt-4o-mini").is_ok());
        REQUIRE(InputValidator::validate_model_name("gemini-2.5-flash").is_ok());
        REQUIRE(InputValidator::validate_model_name("llama-3b").is_ok());
        REQUIRE(InputValidator::validate_model_name("models/gemini-pro").is_ok());
    }
    
    SECTION("Model name with invalid characters fails") {
        auto result = InputValidator::validate_model_name("model<name>");
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::InvalidInput);
    }
}

TEST_CASE("InputValidator - is_reserved_filename", "[validator]") {
    SECTION("Reserved names are detected") {
        REQUIRE(InputValidator::is_reserved_filename("CON"));
        REQUIRE(InputValidator::is_reserved_filename("con"));
        REQUIRE(InputValidator::is_reserved_filename("Con"));
        REQUIRE(InputValidator::is_reserved_filename("PRN"));
        REQUIRE(InputValidator::is_reserved_filename("AUX"));
        REQUIRE(InputValidator::is_reserved_filename("NUL"));
        REQUIRE(InputValidator::is_reserved_filename("COM1"));
        REQUIRE(InputValidator::is_reserved_filename("LPT1"));
    }
    
    SECTION("Reserved names with extension are detected") {
        REQUIRE(InputValidator::is_reserved_filename("CON.txt"));
        REQUIRE(InputValidator::is_reserved_filename("PRN.doc"));
    }
    
    SECTION("Normal names are not reserved") {
        REQUIRE_FALSE(InputValidator::is_reserved_filename("Documents"));
        REQUIRE_FALSE(InputValidator::is_reserved_filename("config"));
        REQUIRE_FALSE(InputValidator::is_reserved_filename("console"));  // Different from CON
        REQUIRE_FALSE(InputValidator::is_reserved_filename("CONNECTION"));
    }
}

TEST_CASE("InputValidator - contains_only_path_safe_chars", "[validator]") {
    SECTION("Normal text is safe") {
        REQUIRE(InputValidator::contains_only_path_safe_chars("Documents"));
        REQUIRE(InputValidator::contains_only_path_safe_chars("My Files"));
        REQUIRE(InputValidator::contains_only_path_safe_chars("file-name_123"));
    }
    
    SECTION("Invalid characters are detected") {
        REQUIRE_FALSE(InputValidator::contains_only_path_safe_chars("test<file>"));
        REQUIRE_FALSE(InputValidator::contains_only_path_safe_chars("test:file"));
        REQUIRE_FALSE(InputValidator::contains_only_path_safe_chars("test\"file"));
        REQUIRE_FALSE(InputValidator::contains_only_path_safe_chars("test|file"));
        REQUIRE_FALSE(InputValidator::contains_only_path_safe_chars("test?file"));
        REQUIRE_FALSE(InputValidator::contains_only_path_safe_chars("test*file"));
    }
}

TEST_CASE("InputValidator - sanitize_filename", "[validator]") {
    SECTION("Normal filenames unchanged") {
        REQUIRE(InputValidator::sanitize_filename("document.txt") == "document.txt");
        REQUIRE(InputValidator::sanitize_filename("My File") == "My File");
    }
    
    SECTION("Empty filename becomes 'unnamed'") {
        REQUIRE(InputValidator::sanitize_filename("") == "unnamed");
    }
    
    SECTION("Invalid characters replaced with underscore") {
        REQUIRE(InputValidator::sanitize_filename("file<name>") == "file_name_");
        REQUIRE(InputValidator::sanitize_filename("test:file") == "test_file");
    }
    
    SECTION("Leading/trailing spaces and dots trimmed") {
        REQUIRE(InputValidator::sanitize_filename("  file  ") == "file");
        REQUIRE(InputValidator::sanitize_filename("...file...") == "file");
    }
    
    SECTION("Reserved names get underscore prefix") {
        REQUIRE(InputValidator::sanitize_filename("CON") == "_CON");
        REQUIRE(InputValidator::sanitize_filename("PRN") == "_PRN");
    }
    
    SECTION("All-space filename becomes 'unnamed'") {
        REQUIRE(InputValidator::sanitize_filename("   ") == "unnamed");
    }
}
