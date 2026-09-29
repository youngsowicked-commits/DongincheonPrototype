#include "Audio/DICharacterAudioProfile.h"

const FDIAudioEventDefinition*
UDICharacterAudioProfile::FindEvent(const FGameplayTag& EventTag) const
{
	return Events.Find(EventTag);
}