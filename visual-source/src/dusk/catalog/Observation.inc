// Caller supplies typed reads for the target and a record(id, reached) callback.
#define CARAPACA_flag(id, value) record(Id::id, CARAPACA_READ_FLAG(value));
#define CARAPACA_item(id, value) record(Id::id, CARAPACA_READ_ITEM(value));
#define CARAPACA_poe(id, value) record(Id::id, poeCount >= value);
#define CARAPACA_event(id, value)
#define CARAPACA_mastery(id, value)
#define CARAPACA_ENTRY(id, name, desc, rarity, optional, rule, value) CARAPACA_##rule(id, value)
#include "Index.inc"
#undef CARAPACA_ENTRY
#undef CARAPACA_flag
#undef CARAPACA_item
#undef CARAPACA_poe
#undef CARAPACA_event
#undef CARAPACA_mastery
