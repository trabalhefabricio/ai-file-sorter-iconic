#include <catch2/catch_test_macros.hpp>
#include "Result.hpp"

#include <string>
#include <vector>

using namespace afs;

// Test helper functions
static Result<int> divide(int a, int b) {
    if (b == 0) {
        return make_error(ErrorCode::InvalidInput, "Division by zero");
    }
    return a / b;
}

static Result<std::string> get_name(bool has_name) {
    if (!has_name) {
        return make_error(ErrorCode::EmptyInput, "No name available");
    }
    return std::string("TestName");
}

static Result<void> validate(bool valid) {
    if (!valid) {
        return make_error(ErrorCode::InvalidInput, "Validation failed");
    }
    return ok();
}

TEST_CASE("Result - Value Operations", "[result]") {
    SECTION("Creating result with value") {
        Result<int> result = 42;
        
        REQUIRE(result.has_value());
        REQUIRE(result.is_ok());
        REQUIRE_FALSE(result.is_error());
        REQUIRE(static_cast<bool>(result) == true);
        REQUIRE(result.value() == 42);
        REQUIRE(*result == 42);
    }
    
    SECTION("Creating result with string value") {
        Result<std::string> result = std::string("hello");
        
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "hello");
    }
    
    SECTION("Creating result with vector value") {
        Result<std::vector<int>> result = std::vector<int>{1, 2, 3};
        
        REQUIRE(result.has_value());
        REQUIRE(result.value().size() == 3);
    }
}

TEST_CASE("Result - Error Operations", "[result]") {
    SECTION("Creating result with error") {
        Result<int> result = make_error(ErrorCode::InvalidInput, "Test error", "Details");
        
        REQUIRE_FALSE(result.has_value());
        REQUIRE_FALSE(result.is_ok());
        REQUIRE(result.is_error());
        REQUIRE(static_cast<bool>(result) == false);
        
        const Error& err = result.error();
        REQUIRE(err.code == ErrorCode::InvalidInput);
        REQUIRE(err.message == "Test error");
        REQUIRE(err.details == "Details");
    }
    
    SECTION("Error categories are correctly identified") {
        Error validation_err{ErrorCode::InvalidPath};
        Error filesystem_err{ErrorCode::FileNotFound};
        Error network_err{ErrorCode::NetworkUnavailable};
        Error api_err{ErrorCode::ApiAuthFailed};
        Error db_err{ErrorCode::DatabaseOpenFailed};
        Error llm_err{ErrorCode::LlmLoadFailed};
        Error internal_err{ErrorCode::InternalError};
        
        REQUIRE(validation_err.category() == ErrorCategory::Validation);
        REQUIRE(filesystem_err.category() == ErrorCategory::FileSystem);
        REQUIRE(network_err.category() == ErrorCategory::Network);
        REQUIRE(api_err.category() == ErrorCategory::Api);
        REQUIRE(db_err.category() == ErrorCategory::Database);
        REQUIRE(llm_err.category() == ErrorCategory::LLM);
        REQUIRE(internal_err.category() == ErrorCategory::Internal);
    }
    
    SECTION("Error code names are correct") {
        REQUIRE(std::string(error_code_name(ErrorCode::Ok)) == "OK");
        REQUIRE(std::string(error_code_name(ErrorCode::InvalidPath)) == "InvalidPath");
        REQUIRE(std::string(error_code_name(ErrorCode::FileNotFound)) == "FileNotFound");
    }
}

TEST_CASE("Result - Functional Operations", "[result]") {
    SECTION("Successful division") {
        auto result = divide(10, 2);
        
        REQUIRE(result.is_ok());
        REQUIRE(result.value() == 5);
    }
    
    SECTION("Division by zero returns error") {
        auto result = divide(10, 0);
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::InvalidInput);
        REQUIRE(result.error().message == "Division by zero");
    }
    
    SECTION("value_or provides default on error") {
        auto result = divide(10, 0);
        
        REQUIRE(result.value_or(-1) == -1);
        
        auto success = divide(10, 2);
        REQUIRE(success.value_or(-1) == 5);
    }
    
    SECTION("map transforms successful values") {
        auto result = divide(10, 2);
        auto doubled = result.map([](int v) { return v * 2; });
        
        REQUIRE(doubled.is_ok());
        REQUIRE(doubled.value() == 10);
    }
    
    SECTION("map propagates errors") {
        auto result = divide(10, 0);
        auto doubled = result.map([](int v) { return v * 2; });
        
        REQUIRE(doubled.is_error());
        REQUIRE(doubled.error().code == ErrorCode::InvalidInput);
    }
}

TEST_CASE("Result<void> Operations", "[result]") {
    SECTION("Successful void result") {
        auto result = validate(true);
        
        REQUIRE(result.is_ok());
        REQUIRE_FALSE(result.is_error());
        REQUIRE(static_cast<bool>(result) == true);
    }
    
    SECTION("Failed void result") {
        auto result = validate(false);
        
        REQUIRE_FALSE(result.is_ok());
        REQUIRE(result.is_error());
        REQUIRE(static_cast<bool>(result) == false);
        REQUIRE(result.error().code == ErrorCode::InvalidInput);
    }
    
    SECTION("ok() creates successful void result") {
        Result<void> result = ok();
        
        REQUIRE(result.is_ok());
    }
}

TEST_CASE("Result - Exception Handling", "[result]") {
    SECTION("Accessing value on error throws") {
        Result<int> result = make_error(ErrorCode::InvalidInput, "Test");
        
        REQUIRE_THROWS_AS(result.value(), std::runtime_error);
    }
    
    SECTION("Accessing error on value throws") {
        Result<int> result = 42;
        
        REQUIRE_THROWS_AS(result.error(), std::runtime_error);
    }
    
    SECTION("Void result value() throws on error") {
        Result<void> result = make_error(ErrorCode::InvalidInput, "Test");
        
        REQUIRE_THROWS_AS(result.value(), std::runtime_error);
    }
}

TEST_CASE("Error - Formatting", "[error]") {
    SECTION("Basic error format") {
        Error err{ErrorCode::InvalidPath, "Path is invalid", "Path: /test"};
        std::string formatted = err.format();
        
        REQUIRE(formatted.find("InvalidPath") != std::string::npos);
        REQUIRE(formatted.find("Path is invalid") != std::string::npos);
        REQUIRE(formatted.find("Path: /test") != std::string::npos);
    }
    
    SECTION("Ok error formats as success") {
        Error err{ErrorCode::Ok};
        REQUIRE(err.format() == "Success");
    }
    
    SECTION("Error without details") {
        Error err{ErrorCode::FileNotFound, "File not found"};
        std::string formatted = err.format();
        
        REQUIRE(formatted.find("FileNotFound") != std::string::npos);
        REQUIRE(formatted.find("File not found") != std::string::npos);
    }
}

TEST_CASE("Error - Predicates", "[error]") {
    SECTION("is_ok and is_error") {
        Error ok_err{ErrorCode::Ok};
        Error bad_err{ErrorCode::InvalidInput, "bad"};
        
        REQUIRE(ok_err.is_ok());
        REQUIRE_FALSE(ok_err.is_error());
        
        REQUIRE_FALSE(bad_err.is_ok());
        REQUIRE(bad_err.is_error());
    }
}

TEST_CASE("Result - Chaining", "[result]") {
    SECTION("and_then chains successful operations") {
        auto result = divide(20, 2)
            .and_then([](int v) { return divide(v, 2); });
        
        REQUIRE(result.is_ok());
        REQUIRE(result.value() == 5);
    }
    
    SECTION("and_then short-circuits on first error") {
        auto result = divide(20, 0)
            .and_then([](int v) { return divide(v, 2); });
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().code == ErrorCode::InvalidInput);
    }
    
    SECTION("and_then propagates error from chained operation") {
        auto result = divide(20, 2)
            .and_then([](int v) { return divide(v, 0); });
        
        REQUIRE(result.is_error());
        REQUIRE(result.error().message == "Division by zero");
    }
}

TEST_CASE("Result - Move Semantics", "[result]") {
    SECTION("Moving string result") {
        Result<std::string> result = std::string("test");
        std::string moved = std::move(result).value();
        
        REQUIRE(moved == "test");
    }
    
    SECTION("ok(value) creates result with moved value") {
        std::string original = "hello";
        auto result = ok(std::move(original));
        
        REQUIRE(result.is_ok());
        REQUIRE(result.value() == "hello");
    }
}
