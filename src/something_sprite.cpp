struct Sprite
{
    SDL_Rect srcrect;
    SDL_Texture *texture;
};

#define ARRAY_SIZE(xs) (sizeof(xs) / sizeof(xs[0]))

void render_sprite(SDL_Renderer *renderer,
                   Sprite texture,
                   Rectf destrect,
                   SDL_RendererFlip flip = SDL_FLIP_NONE)
{
    SDL_Rect rect = rectf_for_sdl(destrect);
    sec(SDL_RenderCopyEx(
            renderer,
            texture.texture,
            &texture.srcrect,
            &rect,
            0.0,
            nullptr,
            flip));
}

void render_sprite(SDL_Renderer *renderer,
                   Sprite texture,
                   Vec2f pos,
                   SDL_RendererFlip flip = SDL_FLIP_NONE)
{
    const Rectf destrect = {
        pos.x - (float) texture.srcrect.w * 0.5f,
        pos.y - (float) texture.srcrect.h * 0.5f,
        (float) texture.srcrect.w,
        (float) texture.srcrect.h
    };

    render_sprite(renderer, texture, destrect, flip);
}

struct Frame_Animat
{
    Sprite  *frames;
    size_t   frame_count;
    size_t   frame_current;
    float frame_duration;
    float frame_cooldown;
};

static inline
void render_animat(SDL_Renderer *renderer,
                   Frame_Animat animat,
                   Rectf dstrect,
                   SDL_RendererFlip flip = SDL_FLIP_NONE)
{
    render_sprite(
        renderer,
        animat.frames[animat.frame_current % animat.frame_count],
        dstrect,
        flip);
}

static inline
void render_animat(SDL_Renderer *renderer,
                   Frame_Animat animat,
                   Vec2f pos,
                   SDL_RendererFlip flip = SDL_FLIP_NONE)
{
    render_sprite(
        renderer,
        animat.frames[animat.frame_current % animat.frame_count],
        pos,
        flip);
}

void update_animat(Frame_Animat *animat, float dt)
{
    assert(animat);

    if (dt < animat->frame_cooldown) {
        animat->frame_cooldown -= dt;
    } else {
        animat->frame_current = (animat->frame_current + 1) % animat->frame_count;
        animat->frame_cooldown = animat->frame_duration;
    }
}

SDL_Surface *load_png_file_as_surface(const char *image_filename)
{
    png_image image;
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    // TODO(#6): implement libpng error checker similar to the SDL one
    // TODO(#7): try stb_image.h instead of libpng
    //   https://github.com/nothings/stb/blob/master/stb_image.h
    if (!png_image_begin_read_from_file(&image, image_filename)) {
        println(stderr, "Could not read file `", image_filename, "`: ", image.message);
        abort();
    }
    image.format = PNG_FORMAT_RGBA;
    uint32_t *image_pixels = new uint32_t[image.width * image.height];

    if (!png_image_finish_read(&image, nullptr, image_pixels, 0, nullptr)) {
        println(stderr, "libpng pooped itself: ", image.message);
        abort();
    }

    SDL_Surface* image_surface =
        sec(SDL_CreateRGBSurfaceFrom(image_pixels,
                                     (int) image.width,
                                     (int) image.height,
                                     32,
                                     (int) image.width * 4,
                                     0x000000FF,
                                     0x0000FF00,
                                     0x00FF0000,
                                     0xFF000000));
    return image_surface;
}

SDL_Texture *load_texture_from_png_file(SDL_Renderer *renderer,
                                        const char *image_filename)
{
    SDL_Surface *image_surface =
        load_png_file_as_surface(image_filename);

    SDL_Texture *image_texture =
        sec(SDL_CreateTextureFromSurface(renderer,
                                         image_surface));
    SDL_FreeSurface(image_surface);

    return image_texture;
}

Sprite load_png_file_as_sprite(SDL_Renderer *renderer, const char *image_filename)
{
    Sprite sprite = {};
    sprite.texture = load_texture_from_png_file(renderer, image_filename);
    sec(SDL_QueryTexture(sprite.texture, NULL, NULL, &sprite.srcrect.w, &sprite.srcrect.h));
    return sprite;
}

struct Spritesheet
{
    const char *filename;
    SDL_Texture *texture;
};

Spritesheet spritesheets[] = {
    {"./assets/sprites/Destroy1-sheet.png", nullptr},
    {"./assets/sprites/fantasy_tiles.png", nullptr},
    {"./assets/sprites/spark1-sheet.png", nullptr},
    {"./assets/sprites/walking-12px-zoom.png", nullptr},
};

void load_spritesheets(SDL_Renderer *renderer)
{
    for (size_t i = 0; i < ARRAY_SIZE(spritesheets); ++i) {
        if (spritesheets[i].texture == nullptr) {
            spritesheets[i].texture = load_texture_from_png_file(
                renderer,
                spritesheets[i].filename);
        }
    }
}

SDL_Texture *spritesheet_by_name(String_View filename)
{
    for (size_t i = 0; i < ARRAY_SIZE(spritesheets); ++i) {
        if (filename == cstr_as_string_view(spritesheets[i].filename)) {
            return spritesheets[i].texture;
        }
    }

    println(stderr,
            "[ERROR] Unknown texture file `", filename, "`. ",
            "You may want to add it to the `spritesheets` array.");
    abort();

    return nullptr;
}

Frame_Animat load_spritesheet_animat(SDL_Renderer *renderer,
                                     size_t frame_count,
                                     float frame_duration,
                                     const char *spritesheet_filepath)
{
    Frame_Animat result = {};
    result.frames = new Sprite[frame_count];
    result.frame_count = frame_count;
    result.frame_duration = frame_duration;

    SDL_Texture *spritesheet = load_texture_from_png_file(renderer, spritesheet_filepath);
    int spritesheet_w = 0;
    int spritesheet_h = 0;
    sec(SDL_QueryTexture(spritesheet, NULL, NULL, &spritesheet_w, &spritesheet_h));
    int sprite_w = spritesheet_w / (int) frame_count;
    int sprite_h = spritesheet_h; // NOTE: we only handle horizontal sprites

    for (int i = 0; i < (int) frame_count; ++i) {
        result.frames[i].texture = spritesheet;
        result.frames[i].srcrect = {i * sprite_w, 0, sprite_w, sprite_h};
    }

    return result;
}

void dump_animat(Frame_Animat animat, const char *sprite_filename, FILE *output)
{
    println(output, "sprite = ", sprite_filename);
    println(output, "count = ", animat.frame_count);
    println(output, "duration = ", animat.frame_duration);
    println(output);
    for (size_t i = 0; i < animat.frame_count; ++i) {
        println(output, "frames.", i, ".x = ", animat.frames[i].srcrect.x);
        println(output, "frames.", i, ".y = ", animat.frames[i].srcrect.y);
        println(output, "frames.", i, ".w = ", animat.frames[i].srcrect.w);
        println(output, "frames.", i, ".h = ", animat.frames[i].srcrect.h);
    }
}

void abort_parse_error(FILE *stream,
                       String_View source, String_View rest,
                       const char *prefix, const char *error)
{
    assert(stream);
    assert(source.data < rest.data);

    size_t n = (size_t) (rest.data - source.data);

    for (size_t line_number = 1; source.count; ++line_number) {
        auto line = source.chop_by_delim('\n');

        if (n <= line.count) {
            println(stream, prefix, ':', line_number, ": ", error);
            println(stream, line);
            println(stream, Pad {n, ' '}, '^');
            break;
        }

        n -= line.count + 1;
    }

    for (int i = 0; source.count && i < 3; ++i) {
        auto line = source.chop_by_delim('\n');
        fwrite(line.data, 1, line.count, stream);
        fputc('\n', stream);
    }

    abort();
}

struct Rubber_Animat
{
    Sprite sprite;
    float begin;
    float end;
    float duration;
    float t;

    void render(SDL_Renderer *renderer,
                Vec2f pos,
                Rectf texbox,
                SDL_RendererFlip flip = SDL_FLIP_NONE) const
    {
        const float offset = begin + (end - begin) * (t / duration);
        const float w = texbox.w + offset * texbox.h;
        const float h = texbox.h - offset * texbox.h;
        Rectf dstrect = {pos.x - w * 0.5f, pos.y + (texbox.h * 0.5f) - h, w, h};
        render_sprite(renderer, sprite, dstrect, flip);
    }

    void update(float dt)
    {
        if (!finished()) t += dt;
    }

    bool finished() const
    {
        return t >= duration;
    }

    void reset()
    {
        t = 0.0f;
    }
};

template <size_t N>
struct Compose_Rubber_Animat
{
    Rubber_Animat rubber_animats[N];
    size_t current;

    void render(SDL_Renderer *renderer,
                Vec2f pos,
                Rectf texbox,
                SDL_RendererFlip flip = SDL_FLIP_NONE) const
    {
        rubber_animats[std::min(current, N - 1)].render(renderer, pos, texbox, flip);
    }

    void update(float dt)
    {
        if (finished()) return;

        if (rubber_animats[current].finished()) {
            current += 1;
        }

        rubber_animats[current].update(dt);
    }

    bool finished() const
    {
        return current >= N;
    }

    void reset()
    {
        current = 0;
        for (size_t i = 0; i < N; ++i) {
            rubber_animats[i].reset();
        }
    }
};

struct Squash_Animat
{
    Sprite sprite;
    float duration;
    float a;

    void render(SDL_Renderer *renderer,
                Vec2f pos,
                Rectf texbox,
                SDL_RendererFlip flip = SDL_FLIP_NONE) const
    {
        const float w = texbox.w + texbox.w * a;
        const float h = texbox.h * (1.0f - a);
        Rectf dstrect = {pos.x - w * 0.5f, pos.y + (texbox.h * 0.5f) - h, w, h};
        render_sprite(renderer, sprite, dstrect, flip);
    }

    void update(float dt)
    {
        a += dt / duration;
    }
};

Frame_Animat load_animat_file(const char *animat_filepath)
{
    String_View source = file_as_string_view(animat_filepath);
    String_View input = source;
    Frame_Animat animat = {};
    SDL_Texture *spritesheet_texture = nullptr;

    while (input.count != 0) {
        auto value = input.chop_by_delim('\n');
        auto key = value.chop_by_delim('=').trim();
        if (key.count == 0 || *key.data == '#') continue;
        value = value.trim();

        auto subkey = key.chop_by_delim('.').trim();

        if (subkey == "count"_sv) {
            if (animat.frames != nullptr) {
                abort_parse_error(stderr, source, input, animat_filepath,
                                  "`count` provided twice");
            }

            auto count_result = value.as_integer<int>();
            if (!count_result.has_value) {
                abort_parse_error(stderr, source, input, animat_filepath,
                                  "`count` is not a number");
            }

            animat.frame_count = (size_t) count_result.unwrap;
            animat.frames = new Sprite[animat.frame_count];
        } else if (subkey == "sprite"_sv) {
            spritesheet_texture = spritesheet_by_name(value);
            if (spritesheet_texture == nullptr) {
                println(stderr, "Spritesheet `", value, "` is not loaded. ",
                        "Did you forget to run `load_spritesheets()`?");
                abort();
            }
        } else if (subkey == "duration"_sv) {
            auto result = value.as_integer<int>();
            if (!result.has_value) {
                abort_parse_error(stderr, source, input, animat_filepath,
                                  "`duration` is not a number");
            }

            animat.frame_duration = (float) result.unwrap / 1000.0f;
        } else if (subkey == "frames"_sv) {
            auto result = key.chop_by_delim('.').trim().as_integer<int>();
            if (!result.has_value) {
                abort_parse_error(stderr, source, input, animat_filepath,
                                  "frame index is not a number");
            }

            size_t frame_index = (size_t) result.unwrap;
            if (frame_index >= animat.frame_count) {
                abort_parse_error(stderr, source, input, animat_filepath,
                                  "frame index is bigger than the `count`");
            }

            animat.frames[frame_index].texture = spritesheet_texture;

            while (key.count) {
                subkey = key.chop_by_delim('.').trim();

                if (key.count != 0) {
                    abort_parse_error(stderr, source, input, animat_filepath,
                                      "unknown subkey");
                }

                auto result_value = value.as_integer<int>();
                if (!result_value.has_value) {
                    abort_parse_error(stderr, source, input, animat_filepath,
                                      "value is not a number");
                }

                if (subkey == "x"_sv) {
                    animat.frames[frame_index].srcrect.x = result_value.unwrap;
                } else if (subkey == "y"_sv) {
                    animat.frames[frame_index].srcrect.y = result_value.unwrap;
                } else if (subkey == "w"_sv) {
                    animat.frames[frame_index].srcrect.w = result_value.unwrap;
                } else if (subkey == "h"_sv) {
                    animat.frames[frame_index].srcrect.h = result_value.unwrap;
                } else {
                    abort_parse_error(stderr, source, input, animat_filepath,
                                      "unknown subkey");
                }
            }
        } else {
            abort_parse_error(stderr, source, input, animat_filepath,
                              "unknown subkey");
        }
    }

    delete[] source.data;

    return animat;
}
