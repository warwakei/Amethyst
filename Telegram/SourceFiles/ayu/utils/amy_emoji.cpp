// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#include "ayu/utils/amy_emoji.h"

#include "ayu/utils/amy_name.h"
#include "data/stickers/data_custom_emoji.h"
#include "ui/text/text_utilities.h"

namespace Amethyst {

bool NameHasAmyTag(const QString &name) {
	return name.contains(NamePrefix().trimmed())
		|| name.contains(RetiredNamePrefix().trimmed());
}

TextWithEntities NameWithAmyEmoji(const QString &name) {
	const auto tag = NamePrefix().trimmed();
	const auto retiredTag = RetiredNamePrefix().trimmed();
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

} // namespace Amethyst
