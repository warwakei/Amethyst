#pragma once

#include <QString>

namespace Amethyst {

[[nodiscard]] inline const QString &NamePrefix() {
	static const auto prefix = QString::fromUtf8("[💎Amy] ");
	return prefix;
}

[[nodiscard]] inline const QString &LegacyNamePrefix() {
	static const auto legacy = QString("[Amy] ");
	return legacy;
}

[[nodiscard]] inline QString StripAmyPrefix(const QString &name) {
	auto result = name.trimmed();
	while (true) {
		if (result.startsWith(NamePrefix())) {
			result = result.mid(NamePrefix().size()).trimmed();
		} else if (result.startsWith(LegacyNamePrefix())) {
			result = result.mid(LegacyNamePrefix().size()).trimmed();
		} else {
			break;
		}
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
	return name.startsWith(NamePrefix());
}

} // namespace Amethyst
