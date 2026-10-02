#pragma once

#include <QString>

class UserData;

namespace Amethyst {

[[nodiscard]] inline const QString &NamePrefix() {
	static const auto prefix = QString("[Amy] ");
	return prefix;
}

[[nodiscard]] inline const QString &RetiredNamePrefix() {
	static const auto retired = QString::fromUtf8("[💎Amy] ");
	return retired;
}

[[nodiscard]] inline QString StripAmyPrefix(const QString &name) {
	auto result = name.trimmed();
	while (true) {
		if (result.startsWith(NamePrefix())) {
			result = result.mid(NamePrefix().size()).trimmed();
		} else if (result.startsWith(RetiredNamePrefix())) {
			result = result.mid(RetiredNamePrefix().size()).trimmed();
		} else {
			break;
		}
	}
	if (result == QString("[Amy]")) {
		result.clear();
	}
	return result;
}

[[nodiscard]] inline QString WithAmyPrefix(
		const QString &name,
		int maxLength = 64) {
	const auto base = StripAmyPrefix(name);
	const auto full = NamePrefix() + base;
	if (full.size() <= maxLength) {
		return full;
	}
	return NamePrefix() + base.left(maxLength - NamePrefix().size());
}

[[nodiscard]] inline bool HasAmyPrefix(const QString &name) {
	return name.startsWith(NamePrefix()) || name == QString("[Amy]");
}

void EnforcePrefixOnSelf(UserData *user);

} // namespace Amethyst
