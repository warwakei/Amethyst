// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#include "ayu/utils/amy_emoji.h"

#include "ayu/utils/amy_name.h"
#include "data/stickers/data_custom_emoji.h"
#include "logs.h"
#include "ui/text/text_utilities.h"

namespace Amethyst {
namespace {

[[nodiscard]] QString NormalizedName(const QString &name) {
	return name.normalized(QString::NormalizationForm_KC);
}

[[nodiscard]] TextWithEntities BuildNameWithEmoji(
		const QString &name,
		const QString &tag,
		const QString &retiredTag) {
	const auto emoji = Ui::Text::SingleCustomEmoji(
		Data::SerializeCustomEmojiId(DocumentId(AmyTagEmojiId)),
		tag);
	auto result = TextWithEntities();
	auto rest = name;
	while (true) {
		auto tagPos = -1;
		auto tagLength = 0;
		const auto plainPos = rest.indexOf(tag);
		const auto retiredPos = rest.indexOf(retiredTag);
		if (plainPos >= 0
			&& (retiredPos < 0 || plainPos <= retiredPos)) {
			tagPos = plainPos;
			tagLength = tag.size();
		} else if (retiredPos >= 0) {
			tagPos = retiredPos;
			tagLength = retiredTag.size();
		} else {
			break;
		}
		result.append(rest.mid(0, tagPos));
		result.append(emoji);
		rest = rest.mid(tagPos + tagLength);
	}
	result.append(rest);
	return result;
}

} // namespace

bool NameHasAmyTag(const QString &name) {
	const auto tag = NamePrefix().trimmed();
	if (name.contains(tag)) {
		return true;
	}
	return NormalizedName(name).contains(tag);
}

TextWithEntities NameWithAmyEmoji(const QString &name) {
	const auto tag = NamePrefix().trimmed();
	const auto retiredTag = RetiredNamePrefix().trimmed();
	const auto normalized = NormalizedName(name);
	const auto &source = name.contains(tag) || name.contains(retiredTag)
		? name
		: normalized;
	auto result = BuildNameWithEmoji(source, tag, retiredTag);
	LOG(("AmyEmoji: name with %1 entities.").arg(result.entities.size()));
	return result;
}

} // namespace Amethyst
