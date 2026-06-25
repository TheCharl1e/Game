# L0 Track A — Slice 1 Build Brief (zrekonstruowany)
## Scope
Affordance spine + EQS exploration + physiology foundation. Read-side shelter dampen.
## Zablokowane decyzje
- D4: BP_DayNightCycle = jedyny zegar. C++ konsumuje TimeSpeed + sun przez refleksję, nie trzyma własnego czasu.
- FZoneDef ma TYLKO BaseTemp jako źródło prawdy o temperaturze. Bez duplikowania pól.
- ShelterColdDampenFactor żyje na afordancji w UWorldAffordanceSubsystem, NIE na FZoneDef.
- GetFeltTemperature() = read-side wrapper nad AmbientTemp. NIE wpina się w GetTempQualityMultiplier() ani w ścieżkę hipotermii/śmierci.
- EvaluateCurrentNeed() = czysty czytnik, bez RNG/personality.
## Fail-safes
- No-shelter default = surowy ambient (Dampen=0), nie wartość biased ku komfortowi.
- Wszystkie wskaźniki: IsValid()/if(Pointer) przed użyciem.
## Definition of done
Czysty build Editor + PIE potwierdza: atomowość claimu, cancel-on-death, timer regenu, output EQS.
