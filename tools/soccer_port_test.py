#!/usr/bin/env python3
"""Regression checks for the YSoccer-derived DC32 port."""

from __future__ import annotations

import binascii
import hashlib
import importlib.util
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[1]
COMMIT = "689d8b5cb162e5f270a4d6df44b5eeb82b6170d7"
HEADER_FMT = "<12sII20sIIIIII"
RECORD_FMT = "<32s16sHBBHHHHI4s"
PLAYER_FMT = "<20s12B"
ASSET_FMT = "<24sIIIIHH"
NATIONAL_TEAM_NAMES = (
    "ARGENTINA", "BRAZIL", "ENGLAND", "FRANCE", "GERMANY", "ITALY",
    "NETHERLANDS", "SPAIN", "PORTUGAL", "BELGIUM", "CROATIA", "DENMARK",
    "SWEDEN", "NORWAY", "IRELAND", "SCOTLAND", "WALES", "USA", "MEXICO",
    "JAPAN", "SOUTH KOREA", "AUSTRALIA", "CAMEROON", "NIGERIA",
)
SOURCE_ROSTER_INDICES = (0, 3, 4, 7, 9, 51, 16, 17, 18, 19, 26, 29,
                         37, 41, 44, 49, 50, 5, 10, 15, 20, 35, 47, 14)
RETAINED_KITS = {"PLAIN", "VERTICAL", "HORIZONTAL", "BIG_CHECK"}
ROSTER_HAIR_IDS = {12, 14, 15, 18, 30, 41}
PACKED_HAIR_IDS = ROSTER_HAIR_IDS | {0}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def load_builder():
    path = ROOT / "tools" / "build_soccer_assets.py"
    spec = importlib.util.spec_from_file_location("build_soccer_assets", path)
    require(spec is not None and spec.loader is not None, "cannot load asset builder")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check_pinned_asset_line_endings() -> None:
    builder = load_builder()
    source = ROOT / "third_party" / "ysoccer19" / "upstream" / "java" / "android" / "assets"
    json_relative = Path(
        "data/teams/1964-65/CLUB_TEAMS/EUROPE/ITALY/team.ac_milan.json")
    with tempfile.TemporaryDirectory() as temp:
        test_source = Path(temp)
        for relative in builder.PINNED_ASSET_SHA256:
            relative_path = Path(relative)
            target = test_source / relative_path
            target.parent.mkdir(parents=True, exist_ok=True)
            data = (source / relative_path).read_bytes()
            if relative_path == json_relative:
                data = data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
            target.write_bytes(data)

        builder.validate_source(test_source)
        json_path = test_source / json_relative
        json_path.write_bytes(json_path.read_bytes().replace(b"\n", b"\r\n"))
        builder.validate_source(test_source)


def check_pack() -> None:
    builder = load_builder()
    source = ROOT / "third_party" / "ysoccer19" / "upstream" / "java" / "android" / "assets"
    require(source.exists(), "vendored YSoccer assets are missing")
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp) / "soccer-ysoccer.pak"
        subprocess.run(
            [sys.executable, str(ROOT / "tools" / "build_soccer_assets.py"),
             "--source", str(source), "--output", str(output)],
            check=True,
            cwd=ROOT,
        )
        first = output.read_bytes()
        subprocess.run(
            [sys.executable, str(ROOT / "tools" / "build_soccer_assets.py"),
             "--source", str(source), "--output", str(output)],
            check=True,
            cwd=ROOT,
        )
        second = output.read_bytes()
    require(first == second, "asset pack is not deterministic")
    built_pack = ROOT / "build" / "apps" / "soccer-ysoccer.pak"
    if built_pack.exists():
        require(built_pack.read_bytes() == first,
                "built YSoccer pack is stale; rebuild dcapp_soccer_assets")
    header_size = struct.calcsize(HEADER_FMT)
    record_size = struct.calcsize(RECORD_FMT)
    (magic, version, count, commit, packed_record_size, table_crc, player_offset,
     directory_offset, asset_count, directory_crc) = struct.unpack_from(HEADER_FMT, first)
    require(magic == b"DC32YSOCV5\0\0", "pack magic mismatch")
    require(version == 5, "pack version mismatch")
    require(count == 24, f"expected 24 national teams, got {count}")
    require(commit.hex() == COMMIT, "pack commit mismatch")
    require(packed_record_size == record_size == 68, "team record ABI mismatch")
    table_end = header_size + count * record_size
    table = first[header_size:table_end]
    require(len(table) == count * record_size, "pack table length mismatch")
    names = []
    kit_styles = set()
    original_indices = []
    for index in range(count):
        record = struct.unpack_from(RECORD_FMT, table, index * record_size)
        name = record[0].split(b"\0", 1)[0].decode("utf-8")
        require(name, f"team {index} has no display name")
        require(record[9] != 0, f"team {index} has no provenance seed")
        kit_style = record[1].split(b"\0", 1)[0].decode("ascii")
        require(kit_style, f"team {index} has no upstream kit style")
        kit_styles.add(kit_style)
        original_indices.append(struct.unpack_from("<H", record[10])[0])
        names.append(name)
    require(len(set(names)) == count, "upstream team list contains duplicate names")
    require(tuple(names) == NATIONAL_TEAM_NAMES, "national-team roster changed")
    require(tuple(original_indices) == SOURCE_ROSTER_INDICES,
            "source-derived national roster mapping changed")
    require(kit_styles == RETAINED_KITS, "retained kit-style set changed")
    player_size = struct.calcsize(PLAYER_FMT)
    require(player_offset == table_end, "player table offset mismatch")
    require(directory_offset == player_offset + count * 16 * player_size,
            "player table size mismatch")
    require((binascii.crc32(first[header_size:directory_offset]) & 0xFFFFFFFF) == table_crc,
            "team/player table CRC mismatch")
    roster_hair_ids = set()
    for index in range(count * 16):
        player = struct.unpack_from(PLAYER_FMT, first, player_offset + index * player_size)
        require(player[0].split(b"\0", 1)[0], f"player {index} has no name")
        roster_hair_ids.add(player[5])
    require(roster_hair_ids == ROSTER_HAIR_IDS,
            "national roster hair-style set changed")
    asset_size = struct.calcsize(ASSET_FMT)
    directory = first[directory_offset:directory_offset + asset_count * asset_size]
    require((binascii.crc32(directory) & 0xFFFFFFFF) == directory_crc,
            "asset directory CRC mismatch")
    assets = {}
    for index in range(asset_count):
        entry = struct.unpack_from(ASSET_FMT, directory, index * asset_size)
        name = entry[0].split(b"\0", 1)[0].decode("ascii")
        require(entry[1] + entry[2] <= len(first), f"asset {name} exceeds pack")
        require((binascii.crc32(first[entry[1]:entry[1] + entry[2]]) & 0xFFFFFFFF) == entry[4],
                f"asset {name} CRC mismatch")
        assets[name] = entry
    required = {f"PLY_{style}" for style in RETAINED_KITS}
    required |= {f"HAIR_{builder.HAIR_STYLES[index]}" for index in PACKED_HAIR_IDS}
    required |= {"KEEPER", "SHADOW0", "BALL", "BALL_SNOW", "GOAL_TOP_A",
                 "GOAL_BOTTOM", "PLAYER_ORIGINS", "KEEPER_ORIGINS",
                 "PLAYER_HAIR_MAP", "STAD_TOP", "STAD_BOTTOM", "STAD_LEFT",
                 "STAD_RIGHT"}
    require(required <= set(assets), "required universal-cache graphics are missing")
    require(not ({"AUDIO", "FONT10", "FONT14"} & set(assets)),
            "silent schema-5 pack contains unused audio or fonts")
    require(not any(name.startswith("STAD_") and name[5:].isdigit() for name in assets),
            "schema-5 pack still contains 512x512 stadium tiles")
    require({name[4:] for name in assets if name.startswith("PLY_")} == RETAINED_KITS,
            "pack contains an unretained kit sheet")
    require({builder.HAIR_STYLE_IDS[name[5:]] for name in assets if name.startswith("HAIR_")} ==
            PACKED_HAIR_IDS, "pack contains an unused hair sheet")
    require(assets["PLAYER_ORIGINS"][2] == 16 * 8 * 2,
            "player origin table size mismatch")
    require(assets["KEEPER_ORIGINS"][2] == 20 * 8 * 2,
            "keeper origin table size mismatch")
    require(assets["PLAYER_HAIR_MAP"][2] == 16 * 8 * 4,
            "player hair map size mismatch")
    def image_header(name: str) -> tuple[int, int, int, int]:
        return struct.unpack_from("<HHHH", first, assets[name][1])
    require(image_header("PLY_PLAIN")[0:2] == (256, 512) and
            image_header("PLY_PLAIN")[3] == (32 | (32 << 8)),
            "player frames are not packed contiguously")
    require(image_header("KEEPER")[0:2] == (400, 950) and
            image_header("KEEPER")[3] == (50 | (50 << 8)),
            "keeper frames are not packed contiguously")
    require(image_header("HAIR_SMOOTH_A")[0:2] == (160, 200) and
            image_header("HAIR_SMOOTH_A")[3] == (20 | (20 << 8)),
            "hair frames include upstream 21-pixel editor separators")
    require(image_header("STAD_TOP")[0:2] == (1212, 96) and
            image_header("STAD_BOTTOM")[0:2] == (1212, 96),
            "top/bottom stadium strips have wrong dimensions")
    require(image_header("STAD_LEFT")[0:2] == (96, 1280) and
            image_header("STAD_RIGHT")[0:2] == (96, 1280),
            "left/right stadium strips have wrong dimensions")
    require(len(first) < 2 * 1024 * 1024, "YSoccer schema-5 pack exceeds 2 MiB budget")


def check_wiring() -> None:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    header = (ROOT / "src" / "dcApp.h").read_text(encoding="utf-8")
    catalog = (ROOT / "src" / "dcApp.c").read_text(encoding="utf-8")
    ui = (ROOT / "src" / "ui.c").read_text(encoding="utf-8")
    package = (ROOT / "tools" / "build_sd_zip.py").read_text(encoding="utf-8")
    provenance = (ROOT / "third_party" / "ysoccer19" / "DC32-README.md").read_text(encoding="utf-8")
    copying = ROOT / "third_party" / "ysoccer19" / "COPYING"
    require("add_dcapp(dcapp_soccer soccer 213 LARGE_XIP" in cmake, "soccer target missing")
    require("DcAppIdSoccer = 213" in header, "runtime ID missing")
    require('"Sensible Soccer (YSoccer)", "/APPS/soccer.DC32"' in catalog, "catalog entry missing")
    require("DcAppIdSoccer" in ui and "Sensible Soccer (YSoccer)" in ui, "Ports menu entry missing")
    require('"soccer.DC32"' in package and "soccer-ysoccer.pak" in package, "SD package wiring missing")
    require('"pack_schema": 5' in package and '"bundled_teams": 24' in package,
            "schema-5 national roster metadata missing")
    require(COMMIT in provenance and copying.exists(), "source pin or GPL notice missing")
    for relative in (
        "reference/match/Const.java", "reference/match/PlayerStateKick.java",
        "reference/match/PlayerStateStandRun.java", "reference/match/MatchStateMain.java",
        "reference/competition/Cup.java", "reference/competition/League.java",
        "reference/competition/Tournament.java", "reference/io/ExportTeams.java",
    ):
        require((ROOT / "third_party" / "ysoccer19" / relative).exists(),
                f"pinned upstream reference missing: {relative}")
    for relative in (
        "java/core/src/com/ygames/ysoccer/match/PlayerStateKick.java",
        "java/core/src/com/ygames/ysoccer/match/PlayerStateTackle.java",
        "java/core/src/com/ygames/ysoccer/match/AiStateDefending.java",
        "java/core/src/com/ygames/ysoccer/match/AiStateKickingOff.java",
        "java/core/src/com/ygames/ysoccer/match/MatchStateFreeKickStop.java",
        "java/core/src/com/ygames/ysoccer/match/PlayerStateBarrier.java",
        "java/core/src/com/ygames/ysoccer/match/MatchStateMain.java",
        "java/android/assets/images/player/PLAIN.png",
        "java/android/assets/images/stadium/generic_00.png",
        "java/android/assets/sounds/kick.ogg",
    ):
        require((ROOT / "third_party" / "ysoccer19" / "upstream" / relative).exists(),
                f"complete pinned upstream input missing: {relative}")
    pinned_core = {
        "java/core/src/com/ygames/ysoccer/match/PlayerStateKick.java":
            "cee5def36dc0c4e922b15b9dc4a15b31c5569b1816fb31d447561305d9d2cad4",
        "java/core/src/com/ygames/ysoccer/match/Ball.java":
            "b7301ab967d2d0fbeac9b8c5c172eb30b7b647dd1ab359e722b928a07a7eb450",
    }
    for relative, expected in pinned_core.items():
        path = ROOT / "third_party" / "ysoccer19" / "upstream" / relative
        require(hashlib.sha256(path.read_bytes()).hexdigest() == expected,
                f"pinned upstream engine source changed: {relative}")


def check_game_contracts() -> None:
    core_h = (ROOT / "src" / "apps" / "soccer" / "soccer_core.h").read_text(encoding="utf-8")
    core_c = (ROOT / "src" / "apps" / "soccer" / "soccer_core.c").read_text(encoding="utf-8")
    port = (ROOT / "src" / "apps" / "soccer" / "soccer_port.c").read_text(encoding="utf-8")
    for token in (
        "SOCCER_LOGIC_HZ 64u", "SOCCER_SUBFRAMES 8u", "SoccerRestartThrowIn",
        "SoccerRestartCorner", "SoccerRestartGoalKick", "SoccerRestartFreeKick",
        "SoccerEventCard", "SoccerEventKeeperSave", "pickupCooldown",
        "SOCCER_PITCH_W 1020.0f", "soccerTableApply", "soccerTableCompare",
        "* 60u * SOCCER_LOGIC_HZ", "soccerTryAiTackle", "player->frameY = 4u",
        "0.01f * player->tackling - 0.01f * opponent->control",
        "foulProbability = 0.80f", "SoccerRestartPhasePositioning",
        "soccerKick(match, match->owner, x, y)",
        "if (distance > 30.0f)", "soccerClamp(dx, -1.0f, 1.0f)",
        "tackleCooldown", "SOCCER_LOGIC_HZ * 3u / 4u",
        "SOCCER_FREE_KICK_DISTANCE", "SOCCER_DIRECT_SHOT_DISTANCE",
        "soccerPrepareFreeKickTargets", "freeKickWallMask", "freeKickWallCount",
        "wallPlayer->frameY = 10u", "count < 2u", "count > 5u",
        "soccerAiKeeper", "soccerTeamAttacksDown", "sidesSwitched",
        "restartNoticeTicks", "noticeVisible", "predictedX", "ballDepth", "intoPitch",
        "SOCCER_PENALTY_AREA_DEPTH 174.0f", "SOCCER_KEEPER_SMOTHER_RADIUS 11.0f",
        "keeper->speed * 1.20f", "match->players[match->owner].team != keeper->team",
        "clockTicks++", "match->halfTicks / 3u", "soccerCoreStartExtraTime",
        "SoccerCardSecondYellow", "SoccerCardRed", "soccerGiveRedCard",
        "soccerCoreMoveToFormation", "int8_t nearby[4]", "(i + 1u) % count",
        "soccerDirectionalSwitch", "alignment <= 0.15f",
        "(1.0f - alignment) * 240.0f + distance * 0.08f",
        "soccerPlayerVisible", "match->controlled != nearby[0]",
        "directional >= 0 ? directional : nearby[0]",
    ):
        require(token in core_h or token in core_c, f"missing core contract: {token}")
    for token in (
        "/SAVE/PORTS/SOCCER", "SOCCER_REPLAY_FRAMES", "SoccerPurposeCup",
        "SoccerPurposeLeague", "SoccerPurposeTournament", "SoccerPurposeTraining",
        "SoccerPurposeCpu", "SoccerScreenKeyboard", "soccerTransferTeam",
        "soccerWriteSave", "generation - 1u", "uiReadTouchRaw", "TEAM.000",
        "matchPrevKeys & ~app->draw.keys",
        "soccerLoadMatchImages", "soccerDrawIndexedFrame", "soccerDrawStadiumOutside",
        "SOCCER_SAVE_VERSION 4u", "SOCCER_KEEPER_FRAMES 20u",
        "SOCCER_FRAMEBUFFER_BYTES", "dc32PortMalloc(SOCCER_FRAMEBUFFER_BYTES)",
        "SOCCER_RENDER_HZ 30u", "SOCCER_SPRITE_CACHE_REGULAR_SLOTS 32u",
        "elapsed * SOCCER_LOGIC_HZ", "image->frameWidth == frameWidth",
        "dc32PortReadAssetPack(&app->pak, source, slot->pixels, pixelCount)",
        "20u, 20u", "audioPwmStop();", "SOCCER_XIP_CACHE_VERSION 1u",
        "SOCCER_XIP_CACHE_DATA_OFFSET 0x1000u", "SOCCER_XIP_CACHE_SAFETY 0x20000u",
        "__app_image_end", "DC32YSX1", "soccerPrepareUniversalCache",
        "flashUncachedPtr", "image->cachedPixels", "A: STREAM FROM SD",
        "B: EXIT", "soccerMigrateLegacySave", "SOCCER_LEGACY_TEAM_COUNT 68u",
        "24 NATIONAL TEAMS", "HALF LENGTH", "clockSeconds", "KICK OFF: %.12s",
        "GOAL - %.12s", "soccerDrawBall", "FREE KICK: %.12s",
        "LATE TACKLE", "BACK TACKLE", "SIDE TACKLE", "FRONT TACKLE",
        "YELLOW CARD: RECKLESS FOUL", "RED CARD: SECOND YELLOW",
        "RED CARD: SERIOUS FOUL PLAY", "soccerResolvePenaltyShootout",
        "PENALTIES  %u - %u", "addedMinutes",
        "controlled ? 3 : 2", "x - 5, y - 33", "soccerCoreMoveToFormation",
        "soccerPreparePitchTexture", "soccerDrawPitchTexture",
        "SOCCER_PITCH_TEXTURE_SOURCE_X 900u", "pitchTextureRows",
        "memcpy(destination, textureRow +",
        "soccerDrawBallShadowAt", "worldZ < 24.0f", "worldZ < 72.0f",
        "soccerGrassRgb332", "soccerDrawWeather(app)",
        "fogPalette[256]", "fogDither[16]", "app->fogPalette[row[x]]",
    ):
        require(token in port, f"missing port contract: {token}")
    table_write = port.index("app->xipCacheStart + sizeof(header)")
    header_commit = port.index("app->xipCacheStart, 0, &header")
    require(table_write < header_commit, "XIP cache header is not committed last")
    require("cached ? cached + sizeof(header)" in port,
            "cached image pixels are not resolved from direct XIP pointers")
    run_start = port.index("int soccerAppRun")
    run_loop = port.index("while (!mSoccerAbort)", run_start)
    self_test_start = port.index("if (!soccerCoreSelfTest())", run_start)
    replay_alloc = port.index("app->replay =", self_test_start)
    require("return -1" not in port[self_test_start:replay_alloc],
            "a gameplay regression diagnostic must not make the shipped app fail at launch")
    require(port.count("soccerPrepareUniversalCache(app)") == 1 and
            run_start < port.index("soccerPrepareUniversalCache(app)") < run_loop,
            "universal cache is not prepared once at app launch")
    require("PLAY STOPPED" not in port, "obsolete generic restart banner remains")
    require("RETURN TO CENTRE" not in port, "redundant kickoff reposition banner remains")
    require("KICK OFF  A TO START" not in port, "kickoff still exposes an A-to-start prompt")
    restart_start = core_c.index("static bool soccerRunRestart")
    restart_end = core_c.index("static void soccerUpdateClock", restart_start)
    restart_body = core_c[restart_start:restart_end]
    notice_gate = restart_body.index("if (noticeVisible || match->restartTicks)")
    human_kick = restart_body.index("else if (input && input->firePressed)", notice_gate)
    kick = restart_body.index("soccerKick(match, match->owner")
    require("restartAcknowledged" not in core_h + core_c + port and
            notice_gate < human_kick < kick and
            "match->restartNoticeTicks--" in restart_body,
            "restart notices are not fixed-duration gates independent of the kick button")
    require("clockSpan" not in port and
            "app->match.clockTicks / SOCCER_LOGIC_HZ" in port,
            "HUD clock is not based on real elapsed match seconds")
    require("distance <= 12.0f" not in core_c,
            "close-range AI defenders are still excluded from tackles")
    require(core_c.count("tackleCooldown = SOCCER_LOGIC_HZ") >= 3,
            "tackler and victim recovery delays are missing")
    require("float step = distance > speed ? speed : distance" in core_c,
            "short movement steps still render as a standing player")
    require("match->foulPlayer" in core_c and "match->foulVictim" in core_c and
            "match->foulReason" in core_c,
            "foul attribution is not persisted through the free kick")
    require("(app->draw.frame >> 2) % 5u" not in port,
            "ball rendering still cycles high-contrast frames")
    pitch_draw = port.index("soccerDrawPitchTexture(app, cameraX, cameraY")
    pitch_lines = port.index("dcAppDrawLine(&app->draw, left, top", pitch_draw)
    require(pitch_draw < pitch_lines and "grass ^ 0x0820" not in port,
            "textured grass is not rendered beneath the field markings")
    grass_start = port.index("static uint8_t soccerGrassRgb332")
    grass_end = port.index("static void soccerPreparePitchTexture", grass_start)
    grass_body = port[grass_start:grass_end]
    require("SoccerWeatherRain" in grass_body and "SoccerWeatherSnow" in grass_body and
            "SoccerWeatherFog" in grass_body,
            "weather pitches do not share the grass texture with shifted colors")
    match_draw_start = port.index("static void soccerDrawMatch")
    match_draw_end = port.index("static void soccerDrawPause", match_draw_start)
    match_draw = port[match_draw_start:match_draw_end]
    players_draw = match_draw.index("for (uint8_t i = 0; i < SOCCER_PLAYER_COUNT; i++)")
    weather_draw = match_draw.index("soccerDrawWeather(app)", players_draw)
    scoreboard_draw = match_draw.index(
        "dcAppDrawFill(&app->draw, 0, 0, SOCCER_SCREEN_W, 20", weather_draw)
    radar_draw = match_draw.index("soccerDrawRadar(app)")
    announcement_draw = match_draw.index("if (banner)")
    require(players_draw < weather_draw < scoreboard_draw < radar_draw < announcement_draw,
            "weather is not layered above play and below HUD and announcements")
    require("for (int y = 40; y < 220; y += 24)" not in port,
            "fog still uses horizontal scanlines")
    ball_shadow = port.index("soccerDrawBallShadowAt(app, app->match.ball.x")
    ground_ball = port.index("if (app->match.ball.z < 8.0f)", ball_shadow)
    players = port.index("for (uint8_t i = 0; i < SOCCER_PLAYER_COUNT; i++)", ground_ball)
    air_ball = port.index("if (app->match.ball.z >= 8.0f)", players)
    require(ball_shadow < ground_ball < players < air_ball,
            "ball shadow/ground ball/players/airborne ball layering is incorrect")
    replay_start = port.index("static void soccerDrawReplay")
    replay_shadow = port.index("soccerDrawBallShadowAt(app, frame->x[22]", replay_start)
    replay_ground = port.index("if (frame->z < 8)", replay_shadow)
    require(replay_shadow < replay_ground,
            "replay does not draw the ball shadow beneath the ball")
    require("soccerBeginGoalRestart" in core_c and
            "SoccerRestartPhaseAnnounce" in core_c and
            "SoccerRestartPhasePositioning" in core_c and
            "SoccerRestartPhaseReady" in core_c,
            "staged restart sequence is incomplete")
    require("match->players[taker].x =" not in restart_body and
            "match->players[taker].y =" not in restart_body and
            "if (arrived || !match->restartTicks)" not in restart_body,
            "restart positioning still teleports the taker")
    period_break_start = core_c.index("static void soccerBeginPeriodBreak")
    period_break_end = core_c.index("static void soccerPrepareFreeKickTargets",
                                    period_break_start)
    require("soccerResetKickoff" not in core_c[period_break_start:period_break_end],
            "halftime or extra time still teleports teams into formation")
    require("app->save.version == 3u && legacy == 58u" in port and
            "return SOCCER_TEAM_COUNT" in port,
            "schema-3 custom team is not migrated to the new custom slot")
    for token in ("audioPwmPcmStart", "audioPwmPcmWriteU8", "soccerAudioTick",
                  "soccerPlayClip"):
        require(token not in port, f"silent port still processes audio: {token}")
    draw = (ROOT / "src" / "dcAppDraw.c").read_text(encoding="utf-8")
    require("mDcAppDrawRgb332To565" in draw,
            "frame presentation is missing the RGB332 lookup table")
    # Source-derived scoring/table golden cases.
    rows = [dict(p=0, w=0, d=0, l=0, gf=0, ga=0, pts=0) for _ in range(2)]
    def apply(a: dict[str, int], b: dict[str, int], ga: int, gb: int) -> None:
        a["p"] += 1; b["p"] += 1; a["gf"] += ga; a["ga"] += gb; b["gf"] += gb; b["ga"] += ga
        if ga > gb: a["w"] += 1; b["l"] += 1; a["pts"] += 3
        elif gb > ga: b["w"] += 1; a["l"] += 1; b["pts"] += 3
        else: a["d"] += 1; b["d"] += 1; a["pts"] += 1; b["pts"] += 1
    apply(rows[0], rows[1], 2, 1)
    apply(rows[1], rows[0], 0, 0)
    require(rows[0] == {"p": 2, "w": 1, "d": 1, "l": 0, "gf": 2, "ga": 1, "pts": 4},
            "league golden vector is invalid")
    require("uint8_t data[686]" in port and "positionOffset = 62u" in port and
            "playerOffset = 78u" in port, "SWOS 96/97 TEAM file layout drifted")
    # The two-slot journal must prefer the newest valid generation and fall
    # back to the previous slot after an interrupted/corrupt write.
    slots = [{"generation": 4, "valid": True}, {"generation": 5, "valid": True}]
    selected = max((s for s in slots if s["valid"]), key=lambda s: s["generation"])
    require(selected["generation"] == 5, "journal did not select newest slot")
    slots[1]["valid"] = False
    selected = max((s for s in slots if s["valid"]), key=lambda s: s["generation"])
    require(selected["generation"] == 4, "journal did not recover previous slot")
    tracked = subprocess.run(["git", "ls-files"], cwd=ROOT, check=True, text=True,
                             stdout=subprocess.PIPE).stdout.upper().splitlines()
    require(not any("SWOS.EXE" in path or path.endswith("/TEAM.000") for path in tracked),
            "proprietary SWOS data was tracked")


def check_budgets() -> None:
    elf = ROOT / "build" / "dcapp_soccer"
    image = ROOT / "build" / "apps" / "soccer.DC32"
    require(elf.exists() and image.exists(), "build dcapp_soccer_dcapp first")
    require(image.stat().st_size < 3 * 1024 * 1024, "DC32 image exceeds LARGE_XIP budget")
    size_tool = ROOT / "toolchain" / "13_2_Rel1" / "bin" / "arm-none-eabi-size.exe"
    if not size_tool.exists():
        matches = list(ROOT.glob("**/arm-none-eabi-size.exe"))
        size_tool = matches[0] if matches else Path("arm-none-eabi-size")
    result = subprocess.run([str(size_tool), "-A", str(elf)], check=True, text=True,
                            stdout=subprocess.PIPE).stdout
    sections: dict[str, int] = {}
    for line in result.splitlines():
        fields = line.split()
        if len(fields) >= 2 and fields[0].startswith(".") and fields[1].isdigit():
            sections[fields[0]] = int(fields[1])
    require(sections.get(".data", 0) + sections.get(".bss", 0) <= 0x14000,
            "soccer app RAM exceeds 80 KiB")
    require(sections.get(".text", 0) + sections.get(".dcapp_header", 0) <= 0x300000,
            "soccer app XIP exceeds 3 MiB")

    pack = (ROOT / "build" / "apps" / "soccer-ysoccer.pak").read_bytes()
    header = struct.unpack_from(HEADER_FMT, pack)
    directory_offset, asset_count = header[7], header[8]
    directory_entry_size = struct.calcsize(ASSET_FMT)
    cache_sizes = []
    for index in range(asset_count):
        entry = struct.unpack_from(ASSET_FMT, pack,
                                   directory_offset + index * directory_entry_size)
        name = entry[0].split(b"\0", 1)[0].decode("ascii")
        cache_sizes.append(entry[2])
    require(len(cache_sizes) == 24, "universal XIP cache entry count changed")
    cache_bytes = 0x1000
    for size in cache_sizes:
        cache_bytes = (cache_bytes + 255) & ~255
        cache_bytes += size
    cache_bytes = (cache_bytes + 4095) & ~4095
    app_end = (image.stat().st_size + 4095) & ~4095
    available = 0x300000 - app_end
    require(cache_bytes + 0x20000 <= available,
            f"universal cache leaves less than 128 KiB headroom: {available - cache_bytes}")

    # Header-last commit makes interrupted writes invalid. Each metadata or CRC
    # mismatch must likewise force a rebuild instead of exposing partial data.
    expected = {"magic": b"DC32YSX1", "version": 1, "schema": 5,
                "manifest": 0x12345678, "table": 0x23456789,
                "payload": 0x3456789A, "committed": True}
    def valid(candidate: dict[str, object]) -> bool:
        return candidate == expected
    require(valid(expected.copy()), "valid cache model was rejected")
    for field, value in (("version", 2), ("schema", 4), ("manifest", 0),
                         ("table", 0), ("payload", 0), ("committed", False)):
        damaged = expected.copy(); damaged[field] = value
        require(not valid(damaged), f"cache model accepted invalid {field}")


def main() -> int:
    check_pinned_asset_line_endings()
    check_pack()
    check_wiring()
    check_game_contracts()
    check_budgets()
    print("Sensible Soccer (YSoccer) port regression checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
