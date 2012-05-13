#include "cg_local.h"

// sta acqu-sdk (issue 2): CHRUKER: b086 - Helmet insignias glowed in the dark
rankicon_t rankicons[NUM_EXPERIENCE_LEVELS][2] = {
	{ 
		{ 0, "gfx/hud/ranks/rank1", 128, 128 },  
		{ 0, "bugfix/ranks/rank1", 128, 128 } 
	},
	{ 
		{ 0, "gfx/hud/ranks/rank2", 128, 128 },  
		{ 0, "bugfix/ranks/rank2", 128, 128 } 
	},
	{ 
		{ 0, "gfx/hud/ranks/rank3", 128, 128 },  
		{ 0, "bugfix/ranks/rank3", 128, 128 } 
	},
	{ 
		{ 0, "gfx/hud/ranks/rank4", 128, 128 },  
		{ 0, "bugfix/ranks/rank4", 128, 128 } 
	},
	{ 
		{ 0, "gfx/hud/ranks/rank5", 128, 128 }, 
		{ 0, "bugfix/ranks/rank5", 128, 128 } 
	},
	{ 
		{ 0, "gfx/hud/ranks/rank6", 128, 128 }, 
		{ 0, "bugfix/ranks/rank6", 128, 128 } 
	},
	{ 
		{ 0, "gfx/hud/ranks/rank7", 128, 128 }, 
		{ 0, "bugfix/ranks/rank7", 128, 128 } 
	},
	{ 
		{ 0, "gfx/hud/ranks/rank8", 128, 128 }, 
		{ 0, "bugfix/ranks/rank8", 128, 128 } 
	},
	{ 
		{ 0, "gfx/hud/ranks/rank9", 128, 128 }, 
		{ 0, "bugfix/ranks/rank9", 128, 128 } 
	},
	{ 
		{ 0, "gfx/hud/ranks/rank10", 128, 128 }, 
		{ 0, "bugfix/ranks/rank10", 128, 128 } 
	},
	{ 
		{ 0, "gfx/hud/ranks/rank11", 128, 128 }, 
		{ 0, "bugfix/ranks/rank11", 128, 128 } 
	},
};

//rankicon_t rankicons[NUM_EXPERIENCE_LEVELS][2] = {
//	{ 
//		{ 0, "gfx/hud/ranks/rank1", 128, 128 },  
//		{ 0, "models/players/temperate/common/rank1", 128, 128 } 
//	},
//	{ 
//		{ 0, "gfx/hud/ranks/rank2", 128, 128 },  
//		{ 0, "models/players/temperate/common/rank2", 128, 128 } 
//	},
//	{ 
//		{ 0, "gfx/hud/ranks/rank3", 128, 128 },  
//		{ 0, "models/players/temperate/common/rank3", 128, 128 } 
//	},
//	{ 
//		{ 0, "gfx/hud/ranks/rank4", 128, 128 },  
//		{ 0, "models/players/temperate/common/rank4", 128, 128 } 
//	},
//	{ 
//		{ 0, "gfx/hud/ranks/rank5", 128, 128 }, 
//		{ 0, "models/players/temperate/common/rank5", 128, 128 } 
//	},
//	{ 
//		{ 0, "gfx/hud/ranks/rank6", 128, 128 }, 
//		{ 0, "models/players/temperate/common/rank6", 128, 128 } 
//	},
//	{ 
//		{ 0, "gfx/hud/ranks/rank7", 128, 128 }, 
//		{ 0, "models/players/temperate/common/rank7", 128, 128 } 
//	},
//	{ 
//		{ 0, "gfx/hud/ranks/rank8", 128, 128 }, 
//		{ 0, "models/players/temperate/common/rank8", 128, 128 } 
//	},
//	{ 
//		{ 0, "gfx/hud/ranks/rank9", 128, 128 }, 
//		{ 0, "models/players/temperate/common/rank9", 128, 128 } 
//	},
//	{ 
//		{ 0, "gfx/hud/ranks/rank10", 128, 128 }, 
//		{ 0, "models/players/temperate/common/rank10", 128, 128 } 
//	},
//	{ 
//		{ 0, "gfx/hud/ranks/rank11", 128, 128 }, 
//		{ 0, "models/players/temperate/common/rank11", 128, 128 } 
//	},
//};
// end acqu-sdk (issue 2): CHRUKER: b086

void CG_LoadRankIcons( void ) {
	int i;

	for( i = 1; i < NUM_EXPERIENCE_LEVELS; i++ ) {
		rankicons[i][0].shader = trap_R_RegisterShaderNoMip( rankicons[i][0].iconname );
		rankicons[i][1].shader = trap_R_RegisterShaderNoMip( rankicons[i][1].iconname );
	}
}
