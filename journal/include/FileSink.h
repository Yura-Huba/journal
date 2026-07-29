#pragma once

#include "ISink.h"

#include <fstream>
#include <mutex>
#include <string>
#include <cstdint>

namespace journal {

/// @brief Файловый sink.
/// @details Открывает файл и записывает в него строки.
class FileSink : public ISink {
public:
	/// @brief Создаёт файловый sink.
	/// @throw std::invalid_argument если путь пустой или указывает на директорию.
	/// @throw std::runtime_error если не удалось открыть файл.
	explicit FileSink(const std::string& filePath,
					  bool flushOnWrite = true);

	FileSink(const FileSink&) = delete;
	FileSink& operator=(const FileSink&) = delete;
	FileSink(FileSink&&) = delete;
	FileSink& operator=(FileSink&&) = delete;

	~FileSink() override = default;

	/// @brief Записывает отформатированное сообщение в файл.
	/// @throw std::runtime_error при ошибке записи.
	/// @par Потокобезопасность
	/// Потокобезопасен.
	void write(const std::string& formattedMessage) override;

	/// @brief Проверяет, открыт ли файл.
	/// @return true, если файл открыт и поток жив.
	/// @par Потокобезопасность
	/// Потокобезопасен.
	bool isValid() const override;

protected:
	// Все protected-методы предполагают, что вызывающий
	// поток удерживает mutex_.
	void writeToFile(const std::string& formattedMessage);

	std::uintmax_t currentFileSize() const noexcept;

	// Устанавливает логический размер файла.
	// Используется после ротации.
	void setCurrentFileSize(std::uintmax_t size) noexcept;

	std::string filePath_;
	std::ofstream outFileStream_;

	mutable std::mutex mutex_;

	// Логический размер файла.
	std::uintmax_t currentFileSize_ = 0;

	// Нужно ли делать flush после каждой записи.
	bool flushOnWrite_;
};

}