
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <SDL.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/stat.h>
#include "file.h"
#include "fs.h"
#include "game.h"
#include "systemstub.h"
#include "util.h"

static const char *USAGE =
	"REminiscence - Flashback Interpreter\n"
	"Usage: %s [OPTIONS]...\n"
	"  --datapath=PATH   Path to data files (default 'DATA')\n"
	"  --savepath=PATH   Path to save files (default '.')\n"
	"  --levelnum=NUM    Start to level, bypass introduction\n"
	"  --windowed        Windowed (4x) display\n"
	"  --language=LANG   Language (fr,en,de,sp,it,jp)\n"
	"  --autosave        Save game state automatically\n"
;

static int detectVersion(FileSystem *fs) {
	static const struct {
		const char *filename;
		int type;
		const char *name;
	} table[] = {
		{ "INTRO.SEQ", kResourceTypeDOS, "DOS CD" },
		{ "MENU1SSI.MAP", kResourceTypeDOS, "DOS SSI" },
		{ "LEVEL1.MAP", kResourceTypeDOS, "DOS" },
		{ "LEVEL1.BNQ", kResourceTypeDOS, "DOS (Demo)" },
		{ "LEVEL1.LEV", kResourceTypeAmiga, "Amiga" },
		{ "DEMO.LEV", kResourceTypeAmiga, "Amiga (Demo)" },
		{ 0, -1, 0 }
	};
	for (int i = 0; table[i].filename; ++i) {
		File f;
		if (f.open(table[i].filename, "rb", fs)) {
			debug(DBG_INFO, "Detected %s version", table[i].name);
			return table[i].type;
		}
	}
	return -1;
}

static Language detectLanguage(FileSystem *fs) {
	static const struct {
		const char *filename;
		Language language;
	} table[] = {
		// PC
		{ "ENGCINE.TXT", LANG_EN },
		{ "FR_CINE.TXT", LANG_FR },
		{ "GERCINE.TXT", LANG_DE },
		{ "SPACINE.TXT", LANG_SP },
		{ "ITACINE.TXT", LANG_IT },
		// Amiga
		{ "FRCINE.TXT", LANG_FR },
		{ 0, LANG_EN }
	};
	for (int i = 0; table[i].filename; ++i) {
		File f;
		if (f.open(table[i].filename, "rb", fs)) {
			return table[i].language;
		}
	}
	warning("Unable to detect language, defaults to English");
	return LANG_EN;
}

Options g_options;
const char *g_caption = "Flashback";

static void initOptions() {
	// defaults
	g_options.bypass_protection = true;
	g_options.enable_password_menu = false;
	g_options.enable_language_selection = false;
	g_options.fade_out_palette = true;
	g_options.use_text_cutscenes = false;
	g_options.use_words_protection = false;
	g_options.use_white_tshirt = false;
	g_options.play_asc_cutscene = false;
	g_options.play_caillou_cutscene = false;
	g_options.play_metro_cutscene = false;
	g_options.play_serrure_cutscene = false;
	g_options.play_carte_cutscene = false;
	g_options.play_gamesaved_sound = false;
	// read configuration file
	struct {
		const char *name;
		bool *value;
	} opts[] = {
		{ "bypass_protection", &g_options.bypass_protection },
		{ "enable_password_menu", &g_options.enable_password_menu },
		{ "enable_language_selection", &g_options.enable_language_selection },
		{ "fade_out_palette", &g_options.fade_out_palette },
		{ "use_tile_data", &g_options.use_tile_data },
		{ "use_text_cutscenes", &g_options.use_text_cutscenes },
		{ "use_words_protection", &g_options.use_words_protection },
		{ "use_white_tshirt", &g_options.use_white_tshirt },
		{ "play_asc_cutscene", &g_options.play_asc_cutscene },
		{ "play_caillou_cutscene", &g_options.play_caillou_cutscene },
		{ "play_metro_cutscene", &g_options.play_metro_cutscene },
		{ "play_serrure_cutscene", &g_options.play_serrure_cutscene },
		{ "play_carte_cutscene", &g_options.play_carte_cutscene },
		{ "play_gamesaved_sound", &g_options.play_gamesaved_sound },
		{ 0, 0 }
	};
	static const char *filename = strcat(SDL_GetBasePath(), "rs.cfg");
	FILE *fp = fopen(filename, "rb");
	if (fp) {
		char buf[256];
		while (fgets(buf, sizeof(buf), fp)) {
			if (buf[0] == '#') {
				continue;
			}
			const char *p = strchr(buf, '=');
			if (p) {
				++p;
				while (*p && isspace(*p)) {
					++p;
				}
				if (*p) {
					const bool value = (*p == 't' || *p == 'T' || *p == '1');
					bool foundOption = false;
					for (int i = 0; opts[i].name; ++i) {
						if (strncmp(buf, opts[i].name, strlen(opts[i].name)) == 0) {
							*opts[i].value = value;
							foundOption = true;
							break;
						}
					}
					if (!foundOption) {
						warning("Unhandled option '%s', ignoring", buf);
					}
				}
			}
		}
		fclose(fp);
	}
}

int main(int argc, char *argv[]) {
	const char *dataPath = strcat(SDL_GetBasePath(), "DATA");
	const char *savePath = SDL_GetPrefPath("org.cyxdown", "fb");
	int levelNum = 0;
	bool fullscreen = true;
	bool autoSave = false;
	int forcedLanguage = -1;
	if (argc == 2) {
		// data path as the only command line argument
		struct stat st;
		if (stat(argv[1], &st) == 0 && S_ISDIR(st.st_mode)) {
			dataPath = strdup(argv[1]);
		}
	}
	while (1) {
		static struct option options[] = {
			{ "datapath",   required_argument, 0, 1 },
			{ "savepath",   required_argument, 0, 2 },
			{ "levelnum",   required_argument, 0, 3 },
			{ "windowed",   no_argument,       0, 4 },
			{ "language",   required_argument, 0, 5 },
			{ "autosave",   no_argument,       0, 6 },
			{ 0, 0, 0, 0 }
		};
		int index;
		const int c = getopt_long(argc, argv, "", options, &index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 1:
			dataPath = strdup(optarg);
			break;
		case 2:
			savePath = strdup(optarg);
			break;
		case 3:
			levelNum = atoi(optarg);
			break;
		case 4:
			fullscreen = false;
			break;
		case 5: {
				static const struct {
					int lang;
					const char *str;
				} languages[] = {
					{ LANG_FR, "FR" },
					{ LANG_EN, "EN" },
					{ LANG_DE, "DE" },
					{ LANG_SP, "SP" },
					{ LANG_IT, "IT" },
					{ LANG_JP, "JP" },
					{ -1, 0 }
				};
				for (int i = 0; languages[i].str; ++i) {
					if (strcasecmp(languages[i].str, optarg) == 0) {
						forcedLanguage = languages[i].lang;
						break;
					}
				}
			}
			break;
		case 6:
			autoSave = true;
			break;
		default:
			printf(USAGE, argv[0]);
			return 0;
		}
	}
	initOptions();
	g_debugMask = DBG_INFO; // DBG_CUT | DBG_VIDEO | DBG_RES | DBG_MENU | DBG_PGE | DBG_GAME | DBG_UNPACK | DBG_COL | DBG_MOD | DBG_SFX | DBG_FILE;
	FileSystem fs(dataPath);
	const int version = detectVersion(&fs);
	if (version == -1) {
		error("Unable to find data files, check that all required files are present");
		return -1;
	}
	const Language language = (forcedLanguage == -1) ? detectLanguage(&fs) : (Language)forcedLanguage;
	SystemStub *stub = SystemStub_SDL_create();
	Game *g = new Game(stub, &fs, savePath, levelNum, (ResourceType)version, language, autoSave);
	stub->init(g_caption, g->_vid._w, g->_vid._h, fullscreen);
	g->run();
	delete g;
	stub->destroy();
	delete stub;
	return 0;
}
