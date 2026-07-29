#include "TestRunner.h"

#include "../journal/include/AsyncJournal.h"
#include "../journal/include/FileSink.h"
#include "../journal/include/Journal.h"
#include "../journal/include/RotatingFileSink.h"
#include "../journal/include/SimpleFormatter.h"

#include <condition_variable>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <regex>

namespace fs = std::filesystem;

const std::string TEST_FILE = "test_journal.txt";

// Удаляет тестовые файлы.
void cleanup() {
	std::error_code ec;

	fs::remove(TEST_FILE, ec);

	for (int i = 1; i <= 10; ++i) {
		fs::remove(TEST_FILE + "." + std::to_string(i), ec);
	}
}

// Вспомогательный sink, который блокирует первую запись.
// Нужен для детерминированной проверки переполнения очереди.
class BlockingSink : public journal::ISink {
public:
	void write(const std::string&) override {
		std::unique_lock<std::mutex> lock(mutex_);

		started_ = true;
		cv_.notify_all();

		cv_.wait(lock, [this] { return released_; });
	}

	void waitUntilStarted() {
		std::unique_lock<std::mutex> lock(mutex_);

		cv_.wait(lock, [this] { return started_; });
	}

	void release() {
		std::lock_guard<std::mutex> lock(mutex_);
		released_ = true;
		cv_.notify_all();
	}

private:
	std::mutex mutex_;
	std::condition_variable cv_;

	bool started_ = false;
	bool released_ = false;
};

// Вход: одно сообщение "Сообщение" с уровнем важности INFO.
// Ожидаемое поведение: журнал записывает строку в формате:
// [DD-MM-YYYY HH:MM:SS] [INFO] Сообщение
TEST(Journal_WritesFormat) {
	cleanup();

	{
		auto sink = std::make_unique<journal::FileSink>(TEST_FILE);
		auto formatter = std::make_unique<journal::SimpleFormatter>();

		journal::Journal testJournal(
			std::move(sink),
			std::move(formatter),
			journal::Importance::INFO
		);

		testJournal.write("Сообщение", journal::Importance::INFO);
	}

	std::ifstream f(TEST_FILE);
	std::string line;
	std::getline(f, line);

	std::regex pattern(
		R"(\[\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2}\] \[INFO\] Сообщение)"
	);

	ASSERT_IS_TRUE(std::regex_match(line, pattern));

	cleanup();
}

// Вход: сначала записывается сообщение INFO, затем сообщение FAULT 
// в журнал с уровнем важности ALERT.
// Ожидаемое поведение: сообщение с уровнем INFO фильтруется,
// сообщение с уровнем FAULT записывается.
TEST(Journal_FiltersByImportance) {
	cleanup();

	{
		auto sink = std::make_unique<journal::FileSink>(TEST_FILE);
		auto formatter = std::make_unique<journal::SimpleFormatter>();

		journal::Journal j(
			std::move(sink),
			std::move(formatter),
			journal::Importance::ALERT
		);

		j.write("Обычная запись", journal::Importance::INFO);
		j.write("Сбой", journal::Importance::FAULT);
	}

	std::ifstream f(TEST_FILE);

	std::string line;
	std::getline(f, line);

	ASSERT_IS_TRUE(line.find("Сбой") != std::string::npos);

	// Нет второй строки
	std::string line2;
	bool hasSecondLine = static_cast<bool>( std::getline(f, line2) );

	ASSERT_IS_TRUE(!hasSecondLine);

	cleanup();
}

// Вход: в журнал с уровнем важности FAULT
// записывается сообщение INFO, затем уровень меняется на INFO
// и записывается ещё одно сообщение INFO.
// Ожидаемое поведение: первое сообщение INFO игнорируется,
// а второе сообщение INFO записывается после изменения уровня.
TEST(Journal_ChangesImportance) {
	cleanup();

	{
		auto sink = std::make_unique<journal::FileSink>(TEST_FILE);
		auto formatter = std::make_unique<journal::SimpleFormatter>();

		journal::Journal j(
			std::move(sink),
			std::move(formatter),
			journal::Importance::FAULT
		);

		j.write("Игнор.", journal::Importance::INFO);

		// Меняем уровень важности
		j.setMinImportance(journal::Importance::INFO);

		// Теперь INFO должен записаться.
		j.write("Не игнор.", journal::Importance::INFO);
	}

	std::ifstream f(TEST_FILE);
	std::string line;
	std::getline(f, line);

	ASSERT_IS_TRUE(line.find("Игнор.") == std::string::npos);
	ASSERT_IS_TRUE(line.find("Не игнор.") != std::string::npos);

	cleanup();
}

// Вход: записывается 51 сообщение в Journal с
// RotatingFileSink с максимальным размером файла 1500 байт
// и 2 архивными копиями.
// Ожидаемое поведение: после нескольких ротаций создаются файлы
// test_journal.txt, test_journal.txt.1, test_journal.txt.2
TEST(RotatingFileSink_RotatesOnSize) {
	cleanup();

	{
		auto sink = std::make_unique<journal::RotatingFileSink>(TEST_FILE, 1500, 2, true);

		auto formatter = std::make_unique<journal::SimpleFormatter>();

		journal::Journal testJournal(std::move(sink), std::move(formatter), journal::Importance::INFO);

		for (int i = 0; i < 51; ++i) {
			testJournal.write("Сообщение номер " + std::to_string(i), journal::Importance::INFO);
		}
	}

	ASSERT_IS_TRUE(fs::exists(TEST_FILE));
	ASSERT_IS_TRUE(fs::exists(TEST_FILE + ".1"));
	ASSERT_IS_TRUE(fs::exists(TEST_FILE + ".2"));
	ASSERT_IS_TRUE(!fs::exists(TEST_FILE + ".3"));
	ASSERT_IS_TRUE(!fs::exists(TEST_FILE + ".4"));
	ASSERT_IS_TRUE(!fs::exists(TEST_FILE + ".5"));

	std::ifstream f(TEST_FILE + ".2");
	std::string line;
	std::getline(f, line);

	ASSERT_IS_TRUE(line.find("Сообщение номер 0") != std::string::npos);

	std::ifstream f1(TEST_FILE + ".1");
	std::getline(f1, line);

	ASSERT_IS_TRUE(line.find("Сообщение номер 25") != std::string::npos);

	std::ifstream f2(TEST_FILE);
	std::getline(f2, line);

	ASSERT_IS_TRUE(line.find("Сообщение номер 50") != std::string::npos);

	cleanup();
}

// Вход: 5 потоков записывают по 100 сообщений через AsyncJournal.
// Ожидаемое поведение: AsyncJournal принимает сообщения
// из нескольких потоков, а после деструктора 
// в файле 500 записанных строк.
TEST(AsyncJournal_Concurrency) {
	cleanup();

	constexpr int numberOfThreads = 5;
	constexpr int messagePerThread = 100;

	{
		auto sink = std::make_unique<journal::FileSink>(TEST_FILE);
		auto formatter = std::make_unique<journal::SimpleFormatter>();

		auto inner = std::make_shared<journal::Journal>(
			std::move(sink),
			std::move(formatter),
			journal::Importance::INFO
		);

		journal::AsyncJournal async(inner);

		std::vector<std::thread> threads;

		for (int i = 0; i < numberOfThreads; ++i) {
			threads.emplace_back([&async, i] () {
				for (int k = 0; k < messagePerThread; ++k) {
					async.write("Поток " + std::to_string(i) +
								" запись " +
								std::to_string(k),
								journal::Importance::INFO);
				}
			});
		}

		for (auto& t : threads) {
			t.join();
		}
	}

	std::ifstream f(TEST_FILE);

	int numberOfLines = 0;
	std::string line;

	while (std::getline(f, line)) {
		if (!line.empty()) {
			++numberOfLines;
		}
	}

	ASSERT_ARE_EQUAL(numberOfLines, numberOfThreads * messagePerThread);

	cleanup();
}

// Вход: записывается 200 сообщений 
// в AsyncJournal с размером очереди 5.
// Ожидаемое поведение: переполнение очереди не приводит к падению,
// файл журнала создаётся.
TEST(AsyncJournal_BoundedQueue) {
	cleanup();

	{
		auto blockingSink = std::make_unique<BlockingSink>();
		BlockingSink* sinkPtr = blockingSink.get();

		auto formatter = std::make_unique<journal::SimpleFormatter>();

		auto inner = std::make_shared<journal::Journal>(
			std::move(blockingSink),
			std::move(formatter),
			journal::Importance::INFO
		);

		journal::AsyncJournal async(inner, 5);

		// Первое сообщение заблокирует рабочий поток.
		async.write("Блокирующее сообщение", journal::Importance::INFO);

		// Ждём, пока рабочий поток действительно начал обработку.
		sinkPtr->waitUntilStarted();

		// Теперь рабочий поток занят.
		// Очередь может принять только 5 сообщений, остальные будут пропущены.
		for (int i = 0; i < 200; ++i) {
			async.write("Сообщение " + std::to_string(i), journal::Importance::INFO);
		}

		ASSERT_IS_TRUE(async.getNumberOfDroppedCommands() == 195);

		// Разблокируем sink, чтобы рабочий поток мог завершить обработку.
		sinkPtr->release();
	}

	cleanup();
}

TEST(FileSink_ThrowsOnInvalidPath) {
	ASSERT_THROWS_EXCEPTION(
		journal::FileSink sink("/dxgeiurth3489t98wuf8hdufh384888888/journal.txt"),
		std::runtime_error
	);
}

TEST(FileSink_ThrowsOnEmptyPath) {
	ASSERT_THROWS_EXCEPTION(journal::FileSink sink(""), std::invalid_argument);
}

TEST(FileSink_ThrowsOnDirectoryPath) {
	ASSERT_THROWS_EXCEPTION(journal::FileSink sink("/tmp"), std::invalid_argument);
}

TEST(SimpleFormatter_IncludesAllFields) {
	journal::SimpleFormatter formatter;

	const std::time_t timestamp = std::time(nullptr);

	const std::string formatted = formatter.format(
		"Тест",
		journal::Importance::ALERT,
		timestamp
	);

	std::regex pattern(R"(\[\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2}\] \[ALERT\] Тест\n)");

	ASSERT_IS_TRUE(std::regex_match(formatted, pattern));
}