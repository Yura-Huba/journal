#pragma once

#include "FileSink.h"

#include <cstddef>
#include <string>

namespace journal {

/// @brief Файловый sink с ротацией.
/// @details При достижении maxFileSize текущий файл переименовывается в имя.1,
///          существующие архивы сдвигаются .1 -> .2, .2 -> .3 и т.д., самый старый удаляется.
class RotatingFileSink final : public FileSink {
public:
	/// @brief Конструктор.
	/// @throw std::invalid_argument если maxFileSize или maxArchives = 0.
	explicit RotatingFileSink(
		const std::string& filePath,
		std::uintmax_t maxFileSize = 10 * 1024 * 1024,
		std::size_t maxArchives = 5,
		bool flushOnWrite = true
	);

	RotatingFileSink(const RotatingFileSink&) = delete;
	RotatingFileSink& operator=(const RotatingFileSink&) = delete;
	RotatingFileSink(RotatingFileSink&&) = delete;
	RotatingFileSink& operator=(RotatingFileSink&&) = delete;

	~RotatingFileSink() override = default;

	/// @brief Записывает отформатированное сообщение в файл и при необходимости выполняет ротацию.
	/// @throw std::runtime_error при ошибке записи или ротации.
	/// @par Потокобезопасность
	/// Потокобезопасен.
	void write(const std::string& formattedMessage) override;

private:
	// Все private-методы предполагают, что вызывающий
	// поток удерживает mutex_.

	// Выполняет полную ротацию.
	void rotate();

	// Сдвигает существующие архивы: .1 -> .2, .2 -> .3 и т.д.
	// Самый старый архив удаляется.
	void shiftFiles();

	// Открывает новый файл журнала после ротации.
	void openNewFile();

	// Пытается заново открыть файл после ошибки ротации.
	void recoverOpen();

	// Возвращает архив .1 на место основного файла.
	void undoRename() noexcept;

	// Максимальный размер файла до ротации.
	std::uintmax_t maxFileSize_;

	std::size_t maxArchives_;
};

}