#include "TestRunner.h"

#include "../app/ConsoleApplication.h"
#include "../journal/include/IJournal.h"

#include <atomic>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// Мок журнала для тестирования приложения без файловой системы.
class MockJournal : public journal::IJournal {
public:
	using journal::IJournal::write;

	struct Entry {
		std::string text;
		journal::Importance importance = journal::Importance::INFO;
		std::time_t timestamp = 0;
	};

	std::vector<Entry> entries;
	journal::Importance minImportance = journal::Importance::INFO;

	void write(
		const std::string& message,
		journal::Importance importance,
		std::time_t timestamp
	) override {
		entries.push_back({message, importance, timestamp});
	}

	void setMinImportance(journal::Importance importance) override {
		minImportance = importance;
	}

	journal::Importance getMinImportance() const override {
		return minImportance;
	}
};

// Вход: две обычные строки без уровня важности
// Ожидаемое поведение: записываются с уровнем по умолчанию INFO.
TEST(ConsoleApplication_HandlesSimpleInput) {
	auto mock = std::make_shared<MockJournal>();

	std::istringstream input("Привет мир\nПривет мир 2\n");
	std::stringstream output;

	app::ConsoleApplication app(mock, input, output);

	std::atomic<bool> flag {false};
	app.run(flag);

	ASSERT_ARE_EQUAL(mock->entries.size(), 2u);

	ASSERT_ARE_EQUAL(mock->entries[0].text, "Привет мир");
	ASSERT_ARE_EQUAL(mock->entries[0].importance, journal::Importance::INFO);

	ASSERT_ARE_EQUAL(mock->entries[1].text, "Привет мир 2");
	ASSERT_ARE_EQUAL(mock->entries[1].importance, journal::Importance::INFO);
}

// Вход: две строки c уровнем важности ALERT и FAULT
// Ожидаемое поведение: приложение распознаёт уровень важности
// в начале строки и отделит его от текста сообщения.
TEST(ConsoleApplication_HandlesImportance) {
	auto mock = std::make_shared<MockJournal>();

	std::istringstream input("ALERT Внимание\nFAULT Сбой\n");
	std::stringstream output;

	app::ConsoleApplication app(mock, input, output);

	std::atomic<bool> flag {false};
	app.run(flag);

	ASSERT_ARE_EQUAL(mock->entries.size(), 2u);

	ASSERT_ARE_EQUAL(mock->entries[0].importance, journal::Importance::ALERT);
	ASSERT_ARE_EQUAL(mock->entries[0].text, "Внимание");

	ASSERT_ARE_EQUAL(mock->entries[1].importance, journal::Importance::FAULT);
	ASSERT_ARE_EQUAL(mock->entries[1].text, "Сбой");
}

// Вход: строка, начинающаяся с неизвестного слова "HIGH".
// Ожидаемое поведение: вся строка записывается как обычный текст.
TEST(ConsoleApplication_HandlesUnknownImportance) {
	auto mock = std::make_shared<MockJournal>();

	std::istringstream input("HIGH это не уровень важности\n");
	std::stringstream output;

	app::ConsoleApplication app(mock, input, output);

	std::atomic<bool> flag {false};
	app.run(flag);

	ASSERT_ARE_EQUAL(mock->entries.size(), 1u);

	ASSERT_ARE_EQUAL(mock->entries[0].text, "HIGH это не уровень важности");
	ASSERT_ARE_EQUAL(mock->entries[0].importance, journal::Importance::INFO);
}

// Вход: несколько пустых строк (с спец. символами).
// Ожидаемое поведение: приложение игнорирует пустые строки
// и не создаёт запись в журнале.
TEST(ConsoleApplication_HandlesEmptyLines) {
	auto mock = std::make_shared<MockJournal>();

	std::istringstream input("   \n\t\n\t\t\t \t \t\t\n \n\n \n\n\n");
	std::stringstream output;

	app::ConsoleApplication app(mock, input, output);

	std::atomic<bool> flag {false};
	app.run(flag);

	ASSERT_ARE_EQUAL(mock->entries.size(), 0u);
}

// Вход: команда "!set FAULT", затем обычное сообщение "Текст FAULT".
// Ожидаемое поведение: приложение меняет уровень важности по умолчанию на FAULT,
// выводит сообщение об успешном изменении уровня
// и следущие сообщение с уровнем важности FAULT.
TEST(ConsoleApplication_HandlesSetCommand) {
	auto mock = std::make_shared<MockJournal>();

	std::istringstream input("!set FAULT\nТекст FAULT\n");
	std::stringstream output;

	app::ConsoleApplication app(mock, input, output);

	std::atomic<bool> flag {false};
	app.run(flag);

	ASSERT_ARE_EQUAL(mock->minImportance, journal::Importance::FAULT);

	const std::string outString = output.str();

	ASSERT_IS_TRUE(outString.find("изменен на: FAULT") != std::string::npos);

	ASSERT_ARE_EQUAL(mock->entries.size(), 1u);

	ASSERT_ARE_EQUAL(mock->entries[0].text, "Текст FAULT");
	ASSERT_ARE_EQUAL(mock->entries[0].importance, journal::Importance::FAULT);
}