/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <SDL.h>
#include <SDL_gpu.h>
#include "systemstub.h"
#include "util.h"

static const int kAudioHz = 22050;

static const int kJoystickIndex = 0;
static const int kJoystickCommitValue = 3200;

static const uint32_t kPixelFormat = SDL_PIXELFORMAT_ABGR8888;

struct SystemStub_SDL : SystemStub {
	SDL_Window *_window;
    GPU_Target *_renderer;
    GPU_Image* _texture;
    uint32_t _shader;
    GPU_ShaderBlock _block;
	SDL_GameController *_controller;
	SDL_PixelFormat *_fmt;
	const char *_caption;
	uint32_t *_screenBuffer;
	bool _fullscreen;
	uint8_t _overscanColor;
	uint32_t _rgbPalette[256];
	uint32_t _darkPalette[256];
	int _screenW, _screenH;
	SDL_Joystick *_joystick;
	bool _fadeOnUpdateScreen;
	void (*_audioCbProc)(void *, int16_t *, int);
	void *_audioCbData;

	virtual ~SystemStub_SDL() {}
	virtual void init(const char *title, int w, int h, bool fullscreen);
	virtual void destroy();
	virtual void setScreenSize(int w, int h);
	virtual void setPalette(const uint8_t *pal, int n);
	virtual void getPalette(uint8_t *pal, int n);
	virtual void setPaletteEntry(int i, const Color *c);
	virtual void getPaletteEntry(int i, Color *c);
	virtual void setOverscanColor(int i);
	virtual void copyRect(int x, int y, int w, int h, const uint8_t *buf, int pitch);
	virtual void copyRectRgb24(int x, int y, int w, int h, const uint8_t *rgb);
	virtual void fadeScreen();
	virtual void updateScreen(int shakeOffset);
	virtual void processEvents();
	virtual void sleep(int duration);
	virtual uint32_t getTimeStamp();
	virtual void startAudio(AudioCallback callback, void *param);
	virtual void stopAudio();
	virtual uint32_t getOutputSampleRate();
	virtual void lockAudio();
	virtual void unlockAudio();

	void setPaletteColor(int color, int r, int g, int b);
	void processEvent(const SDL_Event &ev, bool &paused);
	void prepareGraphics();
	void cleanupGraphics();
	void changeGraphics(bool fullscreen);
	void drawRect(int x, int y, int w, int h, uint8_t color);
};

SystemStub *SystemStub_SDL_create() {
	return new SystemStub_SDL();
}

void SystemStub_SDL::init(const char *title, int w, int h, bool fullscreen) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
    if (fullscreen) {
        SDL_ShowCursor(SDL_DISABLE);
    }
	_caption = title;
	memset(&_pi, 0, sizeof(_pi));
	_window = 0;
	_renderer = 0;
	_texture = 0;
	_fmt = SDL_AllocFormat(kPixelFormat);
	_screenBuffer = 0;
	_fadeOnUpdateScreen = false;
	_fullscreen = fullscreen;
	memset(_rgbPalette, 0, sizeof(_rgbPalette));
	memset(_darkPalette, 0, sizeof(_darkPalette));
	_screenW = _screenH = 0;
	setScreenSize(w, h);
	_joystick = 0;
	_controller = 0;
	if (SDL_NumJoysticks() > 0) {
		SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
		if (SDL_IsGameController(kJoystickIndex)) {
			_controller = SDL_GameControllerOpen(kJoystickIndex);
		}
		if (!_controller) {
			_joystick = SDL_JoystickOpen(kJoystickIndex);
		}
	}
}

void SystemStub_SDL::destroy() {
	cleanupGraphics();
	if (_screenBuffer) {
		free(_screenBuffer);
		_screenBuffer = 0;
	}
	if (_fmt) {
		SDL_FreeFormat(_fmt);
		_fmt = 0;
	}
	if (_controller) {
		SDL_GameControllerClose(_controller);
		_controller = 0;
	}
	if (_joystick) {
		SDL_JoystickClose(_joystick);
		_joystick = 0;
	}
	SDL_Quit();
}

void SystemStub_SDL::setScreenSize(int w, int h) {
	if (_screenW == w && _screenH == h) {
		return;
	}
	cleanupGraphics();
	if (_screenBuffer) {
		free(_screenBuffer);
		_screenBuffer = 0;
	}
	const int screenBufferSize = w * h * sizeof(uint32_t);
	_screenBuffer = (uint32_t *)calloc(1, screenBufferSize);
	if (!_screenBuffer) {
		error("SystemStub_SDL::setScreenSize() Unable to allocate offscreen buffer, w=%d, h=%d", w, h);
	}
	_screenW = w;
	_screenH = h;
	prepareGraphics();
}

void SystemStub_SDL::setPaletteColor(int color, int r, int g, int b) {
	_rgbPalette[color] = SDL_MapRGB(_fmt, r, g, b);
	_darkPalette[color] = SDL_MapRGB(_fmt, r / 4, g / 4, b / 4);
}

void SystemStub_SDL::setPalette(const uint8_t *pal, int n) {
	assert(n <= 256);
	for (int i = 0; i < n; ++i) {
		setPaletteColor(i, pal[0], pal[1], pal[2]);
		pal += 3;
	}
}

void SystemStub_SDL::getPalette(uint8_t *pal, int n) {
	assert(n <= 256);
	for (int i = 0; i < n; ++i) {
		SDL_GetRGB(_rgbPalette[i], _fmt, &pal[0], &pal[1], &pal[2]);
		pal += 3;
	}
}

void SystemStub_SDL::setPaletteEntry(int i, const Color *c) {
	setPaletteColor(i, c->r, c->g, c->b);
}

void SystemStub_SDL::getPaletteEntry(int i, Color *c) {
	SDL_GetRGB(_rgbPalette[i], _fmt, &c->r, &c->g, &c->b);
}

void SystemStub_SDL::setOverscanColor(int i) {
	_overscanColor = i;
}

void SystemStub_SDL::copyRect(int x, int y, int w, int h, const uint8_t *buf, int pitch) {
	if (x < 0) {
		x = 0;
	} else if (x >= _screenW) {
		return;
	}
	if (y < 0) {
		y = 0;
	} else if (y >= _screenH) {
		return;
	}
	if (x + w > _screenW) {
		w = _screenW - x;
	}
	if (y + h > _screenH) {
		h = _screenH - y;
	}

	uint32_t *p = _screenBuffer + y * _screenW + x;
	buf += y * pitch + x;

	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			p[i] = _rgbPalette[buf[i]];
		}
		p += _screenW;
		buf += pitch;
	}

	if (_pi.dbgMask & PlayerInput::DF_DBLOCKS) {
		drawRect(x, y, w, h, 0xE7);
	}
}

void SystemStub_SDL::copyRectRgb24(int x, int y, int w, int h, const uint8_t *rgb) {
	assert(x >= 0 && x + w <= _screenW && y >= 0 && y + h <= _screenH);
	uint32_t *p = _screenBuffer + y * _screenW + x;

	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			p[i] = SDL_MapRGB(_fmt, rgb[0], rgb[1], rgb[2]); rgb += 3;
		}
		p += _screenW;
	}

	if (_pi.dbgMask & PlayerInput::DF_DBLOCKS) {
		drawRect(x, y, w, h, 0xE7);
	}
}

void SystemStub_SDL::fadeScreen() {
	_fadeOnUpdateScreen = true;
}

void SystemStub_SDL::updateScreen(int shakeOffset) {
    GPU_Clear(_renderer);

    GPU_UpdateImageBytes(_texture, NULL, (const uint8_t*)_screenBuffer, _screenW * sizeof(uint32_t));

    // *** SHADER DRAW ***
    GPU_ActivateShaderProgram(_shader, &_block);

    int32_t w, h;
    SDL_GetWindowSize(_window, &w, &h);

    const float aspect = _screenW/float(_screenH);
    const float scaleW = w/float(_screenW);
    const float scaleH = h/float(_screenH);

    GPU_Rect rect;
    if (scaleW < scaleH) {
        rect.w = w;
        rect.h = w/aspect;
        rect.x = 0;
        rect.y = (h - rect.h)/2;
    } else {
        rect.w = h*aspect;
        rect.h = h;
        rect.x = (w - rect.w)/2;
        rect.y = 0;
    }

    GPU_SetUniformf(GPU_GetUniformLocation(_shader, "trg_x"), (float)rect.x);
    GPU_SetUniformf(GPU_GetUniformLocation(_shader, "trg_y"), (float)rect.y);
    GPU_SetUniformf(GPU_GetUniformLocation(_shader, "trg_w"), (float)rect.w);
    GPU_SetUniformf(GPU_GetUniformLocation(_shader, "trg_h"), (float)rect.h);
    GPU_SetUniformf(GPU_GetUniformLocation(_shader, "scr_w"), (float)w);
    GPU_SetUniformf(GPU_GetUniformLocation(_shader, "scr_h"), (float)h);

    GPU_Blit(_texture, NULL, _renderer, 0, 0);

    GPU_DeactivateShaderProgram();
    // *******************

    // Mask out the edges with black bars in fullscreen 16:9 mode
    if (_fullscreen) {
        float blackBarWidth = 45.8; // Show some of the extra emulated phosphors on the edges
        float rightStartPos = (float)_screenW - blackBarWidth;

        SDL_Color color = {0, 0, 0, 255};
        GPU_RectangleFilled(_renderer, 0.0, 0.0, blackBarWidth, (float)_screenH, color);
        GPU_RectangleFilled(_renderer, rightStartPos, 0.0, rightStartPos+blackBarWidth, (float)_screenH, color);
    }

    GPU_Flip(_renderer);
}

void SystemStub_SDL::processEvents() {
	bool paused = false;
	while (true) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			processEvent(ev, paused);
			if (_pi.quit) {
				return;
			}
		}
		if (!paused) {
			break;
		}
		SDL_Delay(100);
	}
}

// only used for the protection codes and level passwords
static void setAsciiChar(PlayerInput &pi, const SDL_Keysym *key) {
	if (key->sym >= SDLK_0 && key->sym <= SDLK_9) {
		pi.lastChar = '0' + key->sym - SDLK_0;
	} else if (key->sym >= SDLK_a && key->sym <= SDLK_z) {
		pi.lastChar = 'A' + key->sym - SDLK_a;
	} else if (key->scancode == SDL_SCANCODE_0) {
		pi.lastChar = '0';
	} else if (key->scancode >= SDL_SCANCODE_1 && key->scancode <= SDL_SCANCODE_9) {
		pi.lastChar = '1' + key->scancode - SDL_SCANCODE_1;
	} else if (key->sym == SDLK_SPACE || key->sym == SDLK_KP_SPACE) {
		pi.lastChar = ' ';
	} else {
		pi.lastChar = 0;
	}
}

void SystemStub_SDL::processEvent(const SDL_Event &ev, bool &paused) {
	switch (ev.type) {
	case SDL_QUIT:
		_pi.quit = true;
		break;
	case SDL_WINDOWEVENT:
		switch (ev.window.event) {
		case SDL_WINDOWEVENT_FOCUS_GAINED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			paused = (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST);
			SDL_PauseAudio(paused);
			break;
		}
		break;
	case SDL_JOYHATMOTION:
		if (_joystick) {
			_pi.dirMask = 0;
			if (ev.jhat.value & SDL_HAT_UP) {
				_pi.dirMask |= PlayerInput::DIR_UP;
			}
			if (ev.jhat.value & SDL_HAT_DOWN) {
				_pi.dirMask |= PlayerInput::DIR_DOWN;
			}
			if (ev.jhat.value & SDL_HAT_LEFT) {
				_pi.dirMask |= PlayerInput::DIR_LEFT;
			}
			if (ev.jhat.value & SDL_HAT_RIGHT) {
				_pi.dirMask |= PlayerInput::DIR_RIGHT;
			}
		}
		break;
	case SDL_JOYAXISMOTION:
		if (_joystick) {
			switch (ev.jaxis.axis) {
			case 0:
				_pi.dirMask &= ~(PlayerInput::DIR_RIGHT | PlayerInput::DIR_LEFT);
				if (ev.jaxis.value > kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_RIGHT;
				} else if (ev.jaxis.value < -kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_LEFT;
				}
				break;
			case 1:
				_pi.dirMask &= ~(PlayerInput::DIR_UP | PlayerInput::DIR_DOWN);
				if (ev.jaxis.value > kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_DOWN;
				} else if (ev.jaxis.value < -kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_UP;
				}
				break;
			}
		}
		break;
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		if (_joystick) {
			const bool pressed = (ev.jbutton.state == SDL_PRESSED);
			switch (ev.jbutton.button) {
			case 0:
				_pi.space = pressed;
				break;
			case 1:
				_pi.shift = pressed;
				break;
			case 2:
				_pi.enter = pressed;
				break;
			case 3:
				_pi.backspace = pressed;
				break;
			}
		}
		break;
	case SDL_CONTROLLERAXISMOTION:
		if (_controller) {
			switch (ev.caxis.axis) {
			case SDL_CONTROLLER_AXIS_LEFTX:
			case SDL_CONTROLLER_AXIS_RIGHTX:
				if (ev.caxis.value < -kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_LEFT;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_LEFT;
				}
				if (ev.caxis.value > kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_RIGHT;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
				}
				break;
			case SDL_CONTROLLER_AXIS_LEFTY:
			case SDL_CONTROLLER_AXIS_RIGHTY:
				if (ev.caxis.value < -kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_UP;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_UP;
				}
				if (ev.caxis.value > kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_DOWN;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_DOWN;
				}
				break;
			}
		}
		break;
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
		if (_controller) {
			const bool pressed = (ev.cbutton.state == SDL_PRESSED);
			switch (ev.cbutton.button) {
			case SDL_CONTROLLER_BUTTON_A:
				_pi.enter = pressed;
				break;
			case SDL_CONTROLLER_BUTTON_B:
				_pi.space = pressed;
				break;
			case SDL_CONTROLLER_BUTTON_X:
				_pi.shift = pressed;
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				_pi.backspace = pressed;
				break;
			case SDL_CONTROLLER_BUTTON_BACK:
			case SDL_CONTROLLER_BUTTON_START:
				_pi.escape = pressed;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				if (pressed) {
					_pi.dirMask |= PlayerInput::DIR_UP;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_UP;
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				if (pressed) {
					_pi.dirMask |= PlayerInput::DIR_DOWN;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_DOWN;
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				if (pressed) {
					_pi.dirMask |= PlayerInput::DIR_LEFT;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_LEFT;
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				if (pressed) {
					_pi.dirMask |= PlayerInput::DIR_RIGHT;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
				}
				break;
			}
		}
		break;
	case SDL_KEYUP:
		if (ev.key.keysym.mod & KMOD_ALT) {
			switch (ev.key.keysym.sym) {
			case SDLK_RETURN:
				changeGraphics(!_fullscreen);
				break;
			case SDLK_x:
				_pi.quit = true;
				break;
			}
			break;
		} else if (ev.key.keysym.mod & KMOD_CTRL) {
			switch (ev.key.keysym.sym) {
			case SDLK_f:
				_pi.dbgMask ^= PlayerInput::DF_FASTMODE;
				break;
			case SDLK_b:
				_pi.dbgMask ^= PlayerInput::DF_DBLOCKS;
				break;
			case SDLK_i:
				_pi.dbgMask ^= PlayerInput::DF_SETLIFE;
				break;
			case SDLK_s:
				_pi.save = true;
				break;
			case SDLK_l:
				_pi.load = true;
				break;
			case SDLK_r:
				_pi.rewind = true;
				break;
			case SDLK_KP_PLUS:
			case SDLK_PAGEUP:
				_pi.stateSlot = 1;
				break;
			case SDLK_KP_MINUS:
			case SDLK_PAGEDOWN:
				_pi.stateSlot = -1;
				break;
			}
			break;
		}
		setAsciiChar(_pi, &ev.key.keysym);
		switch (ev.key.keysym.sym) {
		case SDLK_LEFT:
			_pi.dirMask &= ~PlayerInput::DIR_LEFT;
			break;
		case SDLK_RIGHT:
			_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
			break;
		case SDLK_UP:
			_pi.dirMask &= ~PlayerInput::DIR_UP;
			break;
		case SDLK_DOWN:
			_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			break;
		case SDLK_SPACE:
			_pi.space = false;
			break;
		case SDLK_RSHIFT:
		case SDLK_LSHIFT:
			_pi.shift = false;
			break;
		case SDLK_RETURN:
			_pi.enter = false;
			break;
		case SDLK_ESCAPE:
			_pi.escape = false;
			break;
		case SDLK_F1:
		case SDLK_F2:
		case SDLK_F3:
		case SDLK_F4:
		case SDLK_F5:
		case SDLK_F6:
		case SDLK_F7:
		case SDLK_F8:
			break;
		default:
			break;
		}
		break;
	case SDL_KEYDOWN:
		if (ev.key.keysym.mod & (KMOD_ALT | KMOD_CTRL)) {
			break;
		}
		switch (ev.key.keysym.sym) {
		case SDLK_LEFT:
			_pi.dirMask |= PlayerInput::DIR_LEFT;
			break;
		case SDLK_RIGHT:
			_pi.dirMask |= PlayerInput::DIR_RIGHT;
			break;
		case SDLK_UP:
			_pi.dirMask |= PlayerInput::DIR_UP;
			break;
		case SDLK_DOWN:
			_pi.dirMask |= PlayerInput::DIR_DOWN;
			break;
		case SDLK_BACKSPACE:
		case SDLK_TAB:
			_pi.backspace = true;
			break;
		case SDLK_SPACE:
			_pi.space = true;
			break;
		case SDLK_RSHIFT:
		case SDLK_LSHIFT:
			_pi.shift = true;
			break;
		case SDLK_RETURN:
			_pi.enter = true;
			break;
		case SDLK_ESCAPE:
			_pi.escape = true;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void SystemStub_SDL::sleep(int duration) {
	SDL_Delay(duration);
}

uint32_t SystemStub_SDL::getTimeStamp() {
	return SDL_GetTicks();
}

static void mixAudioS16(void *param, uint8_t *buf, int len) {
	SystemStub_SDL *stub = (SystemStub_SDL *)param;
	memset(buf, 0, len);
	stub->_audioCbProc(stub->_audioCbData, (int16_t *)buf, len / 2);
}

void SystemStub_SDL::startAudio(AudioCallback callback, void *param) {
	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));
	desired.freq = kAudioHz;
	desired.format = AUDIO_S16SYS;
	desired.channels = 1;
	desired.samples = 2048;
	desired.callback = mixAudioS16;
	desired.userdata = this;
	if (SDL_OpenAudio(&desired, 0) == 0) {
		_audioCbProc = callback;
		_audioCbData = param;
		SDL_PauseAudio(0);
	} else {
		error("SystemStub_SDL::startAudio() Unable to open sound device");
	}
}

void SystemStub_SDL::stopAudio() {
	SDL_CloseAudio();
}

uint32_t SystemStub_SDL::getOutputSampleRate() {
	return kAudioHz;
}

void SystemStub_SDL::lockAudio() {
	SDL_LockAudio();
}

void SystemStub_SDL::unlockAudio() {
	SDL_UnlockAudio();
}

void SystemStub_SDL::prepareGraphics() {
    //GPU_SetDebugLevel(GPU_DEBUG_LEVEL_MAX);

    int flags = 0;
    if (_fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    flags |= SDL_WINDOW_OPENGL;
    flags |= SDL_WINDOW_ALLOW_HIGHDPI;

    int scale = 4; // Initial scale of non-fullscreen window

	_window = SDL_CreateWindow(_caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _screenW*scale, _screenH*scale, flags);

    GPU_SetInitWindow(SDL_GetWindowID(_window));

    GPU_SetPreInitFlags(GPU_INIT_DISABLE_VSYNC | GPU_INIT_REQUEST_COMPATIBILITY_PROFILE);

    _renderer = GPU_Init(_screenW, _screenH, GPU_DEFAULT_INIT_FLAGS);

    _texture = GPU_CreateImage(_screenW, _screenH, GPU_FORMAT_RGBA);

    GPU_SetAnchor(_texture, 0, 0);
    GPU_SetImageFilter(_texture, GPU_FILTER_NEAREST);

    // *** SHADER SETUP ***
    //GPU_Renderer* renderer = GPU_GetCurrentRenderer();
    //debug(DBG_INFO, "shader_language: %i", renderer->shader_language);
    //debug(DBG_INFO, "max_shader_version: %i", renderer->max_shader_version);

    uint32_t vertex = GPU_LoadShader(GPU_VERTEX_SHADER, "vertex.shader");
    if (!vertex) {
        error("Failed to load vertex shader: %s", GPU_GetShaderMessage());
    }

    uint32_t pixel = GPU_LoadShader(GPU_PIXEL_SHADER, "pixel.shader");
    if (!pixel) {
        error("Failed to load pixel shader: %s", GPU_GetShaderMessage());
    }

    if (_shader) {
        GPU_FreeShaderProgram(_shader);
    }

    _shader = GPU_LinkShaders(vertex, pixel);
    if (_shader) {
        _block = GPU_LoadShaderBlock(_shader, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "gpu_ModelViewProjectionMatrix");
    } else {
        error("Failed to link shader program: %s", GPU_GetShaderMessage());
    }
    // ********************
}

void SystemStub_SDL::cleanupGraphics() {
	if (_texture) {
        GPU_FreeImage(_texture);
		_texture = 0;
	}
	if (_renderer) {
        GPU_Quit();
		_renderer = 0;
	}
	if (_window) {
		SDL_DestroyWindow(_window);
		_window = 0;
	}
}

void SystemStub_SDL::changeGraphics(bool fullscreen) {
	if (fullscreen == _fullscreen) {
		// no change
		return;
	}
	_fullscreen = fullscreen;

	cleanupGraphics();
	prepareGraphics();
}

void SystemStub_SDL::drawRect(int x, int y, int w, int h, uint8_t color) {
	const int x1 = x;
	const int y1 = y;
	const int x2 = x + w - 1;
	const int y2 = y + h - 1;
	assert(x1 >= 0 && x2 < _screenW && y1 >= 0 && y2 < _screenH);
	for (int i = x1; i <= x2; ++i) {
		*(_screenBuffer + y1 * _screenW + i) = *(_screenBuffer + y2 * _screenW + i) = _rgbPalette[color];
	}
	for (int j = y1; j <= y2; ++j) {
		*(_screenBuffer + j * _screenW + x1) = *(_screenBuffer + j * _screenW + x2) = _rgbPalette[color];
	}
}
