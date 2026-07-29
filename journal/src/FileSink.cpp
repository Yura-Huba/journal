#include "../include/FileSink.h"

#include <filesystem>
#include <stdexcept>
#include <system_error>

namespace journal {

namespace fs = std::filesystem;

FileSink::FileSink(const std::string& filePath, bool flushOnWrite)
	: flushOnWrite_(flushOnWrite) {

	if (filePath.empty()) {
		throw std::invalid_argument("FileSink: путь к файлу журнала не может быть пустым");
	}

	std::error_code ec;
	auto resolved = fs::weakly_canonical(filePath, ec);

	if (ec) {
		throw std::invalid_argument("FileSink: не удалось обработать путь '" + filePath + "'. " + ec.message());
	}

	filePath_ = resolved.string();

	// Проверяем, что путь не указывает на существующую директорию
	if (fs::is_directory(filePath_, ec)) {
		throw std::invalid_argument("FileSink: путь указывает на директорию, а не на файл: " + filePath_);
	}

	if (ec && ec != std::errc::no_such_file_or_directory) {
		throw std::runtime_error("FileSink: ошибка проверки пути '" + filePath_ + "': " + ec.message());
	}

	outFileStream_.open(filePath_, std::ios::app);

	if (!outFileStream_.is_open()) {
		throw std::runtime_error("FileSink: не удалось открыть файл журнала: " + filePath_);
	}

	// Определяем начальный размер файла.
	const std::uintmax_t size = fs::file_size(filePath_, ec);

	if (!ec) {
		currentFileSize_ = size;
	} else {
		currentFileSize_ = 0;
	}
}

void FileSink::write(const std::string& formattedMessage) {
	std::lock_guard<std::mutex> lock(mutex_);
	writeToFile(formattedMessage);
}

bool FileSink::isValid() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return outFileStream_.is_open() && outFileStream_;
}

void FileSink::writeToFile(const std::string& formattedMessage) {
	if (!outFileStream_.is_open()) {
		throw std::runtime_error("FileSink: файл журнала закрыт: " + filePath_);
	}

	outFileStream_ << formattedMessage;

	if (flushOnWrite_) {
		outFileStream_.flush();
	}

	if (!outFileStream_) {
		throw std::runtime_error("FileSink: ошибка записи в файл журнала");
	}

	// Обновляем логический размер файла.
	currentFileSize_ += formattedMessage.size();
}

std::uintmax_t FileSink::currentFileSize() const noexcept {
	return currentFileSize_;
}

void FileSink::setCurrentFileSize(std::uintmax_t size) noexcept {
	currentFileSize_ = size;
}

}