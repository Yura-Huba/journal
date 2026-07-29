#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <cstdlib>

namespace testing {

/// @brief Тестовый раннер для тестов без фреймворков.
/// @par Потокобезопасность
/// Не потокобезопасен.
class TestRunner {
public:
	TestRunner(const TestRunner&) = delete;
	TestRunner& operator=(const TestRunner&) = delete;
	TestRunner(TestRunner&&) = delete;
	TestRunner& operator=(TestRunner&&) = delete;

	/// @brief Возвращает синглтон раннера.
	/// @return экземпляр TestRunner.
	static TestRunner& instance() {
		static TestRunner testRunner;
		return testRunner;
	}

	/// @brief Добавляет тест.
	/// @return true если добавлен, false если имя пустое или функция пустая.
	bool addTest(const std::string& name, std::function<void()> function) {
		if (name.empty() || !function) {
			return false;
		}
		tests_.push_back({name, std::move(function)});
		return true;
	}

	/// @brief Запускает все добавленные тесты.
	/// @return EXIT_SUCCESS при успехе, EXIT_FAILURE если были упавшие тесты.
	int run() {
		int passed = 0;
		int failed = 0;

		for (const auto& test : tests_) {
			std::cout << "[ ЗАПУСК   ] " << test.name << "\n";
			try {
				test.function();
				std::cout << "[ ОК       ] " << test.name << "\n";
				++passed;
			} catch (const std::exception& e) {
				std::cout << "[ ОШИБКА   ] " << test.name << " - " << e.what() << "\n";
				++failed;
			} catch (...) {
				std::cout << "[ ОШИБКА   ] " << test.name << "\n";
				++failed;
			}
		}

		std::cout << "\n[==========] " << ( passed + failed ) << " тестов выполнено.\n";

		std::cout << "[ ПРОЙДЕНО ] " << passed << " тестов.\n";

		if (failed > 0) {
			std::cout << "[С ОШИБКАМИ] " << failed << " тестов.\n";
		}

		return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
	}

private:
	TestRunner() = default;
	~TestRunner() = default;

	// Описание теста.
	struct TestCase {
		std::string name;
		std::function<void()> function;
	};

	std::vector<TestCase> tests_;
};

#define TEST(test_name)                                                     \
    static void test_function_##test_name();                                \
    static const bool test_added_##test_name =                              \
        testing::TestRunner::instance().addTest(                            \
            #test_name, test_function_##test_name);                         \
    static void test_function_##test_name()

#define ASSERT_IS_TRUE(expression)                                          \
    do {                                                                    \
        if (!(expression))                                                  \
            throw std::runtime_error(                                       \
                "Утверждение не выполнено: " #expression);                  \
    } while (0)

#define ASSERT_ARE_EQUAL(left, right)                                       \
    do {                                                                    \
        if ((left) != (right))                                              \
            throw std::runtime_error(                                       \
                "Утверждение не выполнено: " #left " == " #right);          \
    } while (0)

#define ASSERT_THROWS_EXCEPTION(expression, exception_type)                 \
    do {                                                                    \
        try {																\
			expression;														\
        } catch (const exception_type&) {									\
			break;															\
        } catch (...) {														\
            throw std::runtime_error(                                       \
                "Ожидалось исключение: " #exception_type);					\
			}																\
        throw std::runtime_error(                                           \
            "Исключение не выброшено: " #expression);                       \
    } while (0)

}