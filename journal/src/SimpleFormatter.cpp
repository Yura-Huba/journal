#include "../include/SimpleFormatter.h"

#include <sstream>
#include <iomanip>

namespace journal {

SimpleFormatter::SimpleFormatter(std::string timestampFormat)
	: timestampFormat_(std::move(timestampFormat)) {}

std::string SimpleFormatter::format(const std::string& message,
									Importance importance,
									std::time_t timestamp) const {
	std::tm tm {};

	// Потокобезопасная версия, в отличие от std::localtime
	if (localtime_r(&timestamp, &tm) == nullptr) {
		// Маловероятно, но всё же...
		return "[Ошибка получения времени] [" + toString(importance) + "] " + message + "\n";
	}

	std::stringstream stream;

	stream << "[" << std::put_time(&tm, timestampFormat_.c_str()) << "] "
		<< "[" << toString(importance) << "] "
		<< message << "\n";
	return stream.str();
}

}