struct PalmAnkleStruct (
	Name,
	Digits = #()
)

Struct LimbStruct (
	Name,
	LMR,
	Bones = #(),
	Collarbone,
	Palm,
	IKTarget
)

struct TailStruct (
	Name,
	Bones = #()
)
struct HubStruct (
	Name,
	Limbs = #(),
	Tails = #(),
	Spines = #()
)