#include "../include/RotatingFileSink.h"

#include <filesystem>
#include <stdexcept>
#include <system_error>

namespace journal {

namespace fs = std::filesystem;

RotatingFileSink::RotatingFileSink(
	const std::string& filePath,
	std::uintmax_t maxFileSize,
	std::size_t maxArchives,
	bool flushOnWrite)
	: FileSink(filePath, flushOnWrite)
	, maxFileSize_(maxFileSize)
	, maxArchives_(maxArchives) {
	if (maxFileSize_ == 0) {
		throw std::invalid_argument("RotatingFileSink: максимальный размер файла должен быть больше нуля");
	}

	if (maxArchives_ == 0) {
		throw std::invalid_argument("RotatingFileSink: количество архивных копий должно быть больше нуля");
	}
}

void RotatingFileSink::write(const std::string& formattedMessage) {
	std::lock_guard<std::mutex> lock(mutex_);

	// Запишем, чтобы не потерять сообщение
	// в случае, если ротация не удастся.
	writeToFile(formattedMessage);

	// Если файл достиг порога, выполняем ротацию.
	if (currentFileSize() >= maxFileSize_) {
		rotate();
	}
}

void RotatingFileSink::rotate() {
	outFileStream_.close();

	if (!outFileStream_) {
		recoverOpen();
		throw std::runtime_error(
			"RotatingFileSink: ошибка закрытия файла журнала перед ротацией: " + filePath_);
	}

	try {
		shiftFiles();
	} catch (...) {
		recoverOpen();
		throw;
	}

	try {
		openNewFile();
	} catch (...) {
		// Не удалось создать новый файл — возвращаем .1 обратно
		undoRename();
		recoverOpen();
		throw;
	}
}

void RotatingFileSink::shiftFiles() {
	std::error_code ec;

	const std::string prefix = filePath_ + ".";

	// Удаляем самый старый архив.
	fs::remove(prefix + std::to_string(maxArchives_), ec);

	// Сдвигаем остальные архивы.
	for (std::size_t i = maxArchives_ - 1; i > 0; --i) {
		const std::string oldName = prefix + std::to_string(i);
		const std::string newName = prefix + std::to_string(i + 1);

		fs::rename(oldName, newName, ec);

		// Отсутствие файла — нормальная ситуация.
		// Любая другая ошибка считается критичной.
		if (ec && ec != std::errc::no_such_file_or_directory) {
			throw std::runtime_error("RotatingFileSink: ошибка сдвига архива при ротации: " + ec.message());
		}
	}

	fs::rename(filePath_, prefix + "1", ec);

	if (ec) {
		throw std::runtime_error("RotatingFileSink: ошибка переименования файла в архив: " + ec.message());
	}
}

void RotatingFileSink::openNewFile() {
	outFileStream_.clear();
	outFileStream_.open(filePath_, std::ios::app);

	if (!outFileStream_.is_open()) {
		throw std::runtime_error("RotatingFileSink: не удалось открыть новый файл журнала после ротации");
	}

	setCurrentFileSize(0);
}

void RotatingFileSink::recoverOpen() {
	outFileStream_.clear();
	outFileStream_.open(filePath_, std::ios::app);

	if (!outFileStream_.is_open()) {
		throw std::runtime_error("RotatingFileSink: не удалось восстановить файл журнала: " + filePath_);
	}

	std::error_code ec;
	const std::uintmax_t size = fs::file_size(filePath_, ec);
	setCurrentFileSize(ec ? 0 : size);
}

void RotatingFileSink::undoRename() noexcept {
	std::error_code ec;
	fs::rename(filePath_ + ".1", filePath_, ec);
}

}