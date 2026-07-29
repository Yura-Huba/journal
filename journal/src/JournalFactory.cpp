#include "../include/JournalFactory.h"
#include "../include/AsyncJournal.h"
#include "../include/Journal.h"
#include "../include/RotatingFileSink.h"
#include "../include/SimpleFormatter.h"

#include <memory>

namespace journal {

IJournalSharedPtr createRotatingFileJournal(const JournalConfig& config) {
	auto sink = std::make_unique<RotatingFileSink>(config.filePath,
												   config.maxFileSize,
												   config.maxArchives,
												   config.flushOnWrite);

	auto formatter = std::make_unique<SimpleFormatter>(config.timestampFormat);

	return std::make_shared<Journal>(std::move(sink),
									 std::move(formatter),
									 config.minImportance);
}

IJournalSharedPtr createAsyncRotatingFileJournal(const JournalConfig& config) {
	auto inner = createRotatingFileJournal(config);

	return std::make_shared<AsyncJournal>(std::move(inner), config.maxQueueSize, config.errorHandler);
}

}