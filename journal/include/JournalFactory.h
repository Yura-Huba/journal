#pragma once

#include "IJournal.h"

#include <cstddef>
#include <string>
#include <cstdint>

namespace journal {

/// @brief Конфиг журнала.
/// @details Используется фабрикой для создания журнала.
struct JournalConfig {
	std::string filePath;

	Importance minImportance = Importance::INFO;

	std::uintmax_t maxFileSize = 10 * 1024 * 1024;

	std::size_t maxArchives = 5;

	std::size_t maxQueueSize = 10000;

	std::string timestampFormat = "%d.%m.%Y %H:%M:%S";

	bool flushOnWrite = true;

	ErrorHandler errorHandler = {};
};

/// @brief Создаёт синхронный журнал с ротацией файлов.
/// @return shared_ptr на созданный журнал.
IJournalSharedPtr createRotatingFileJournal(const JournalConfig& config);

/// @brief Создаёт асинхронный журнал с ротацией файлов.
/// @return shared_ptr на созданный журнал.
IJournalSharedPtr createAsyncRotatingFileJournal(const JournalConfig& config);

}