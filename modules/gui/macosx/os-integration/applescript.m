/*****************************************************************************
 * applescript.m: MacOS X AppleScript support
 *****************************************************************************
 * Copyright (C) 2002-2019 VLC authors and VideoLAN
 *
 * Authors: Derk-Jan Hartman <thedj@users.sourceforge.net>
 *          Felix Paul Kühne <fkuehne at videolan dot org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#import "applescript.h"

#import <vlc_common.h>
#import <vlc_url.h>

#import "main/VLCMain.h"
#import "playqueue/VLCPlayQueueController.h"
#import "playqueue/VLCPlayerController.h"
#import "windows/VLCOpenInputMetadata.h"

/*****************************************************************************
 * VLGetURLScriptCommand implementation
 *****************************************************************************/
@implementation VLGetURLScriptCommand

- (id)performDefaultImplementation
{
    NSString *commandString = [[self commandDescription] commandName];
    NSString *parameterString = [self directParameter];

    if ([commandString isEqualToString:@"GetURL"] || [commandString isEqualToString:@"OpenURL"]) {
        if (parameterString) {
            VLCOpenInputMetadata *inputMetadata = [[VLCOpenInputMetadata alloc] init];
            inputMetadata.MRLString = parameterString;

            [VLCMain.sharedInstance.playQueueController addPlayQueueItems:@[inputMetadata]];
        }
    }
    return nil;
}

@end

/*****************************************************************************
 * VLControlScriptCommand implementation
 *****************************************************************************/
/*
 * This entire control command needs a better design. more object oriented.
 * Applescript developers would be very welcome (hartman)
 */

@implementation VLControlScriptCommand

- (id)performDefaultImplementation
{
    VLCPlayQueueController * const playQueueController = VLCMain.sharedInstance.playQueueController;
    VLCPlayerController * const playerController = [playQueueController playerController];

    NSString * const commandString = [[self commandDescription] commandName];
    NSString * const parameterString = [self directParameter];

    if (commandString == nil || commandString.length == 0) {
        return nil;
    }

    if ([commandString isEqualToString:@"play"]) {
        [playerController togglePlayPause];
    } else if ([commandString isEqualToString:@"stop"]) {
        [playerController stop];
    } else if ([commandString isEqualToString:@"previous"]) {
        [playQueueController playPreviousItem];
    } else if ([commandString isEqualToString:@"next"]) {
        [playQueueController playNextItem];
    } else if ([commandString isEqualToString:@"fullscreen"]) {
        [playerController toggleFullscreen];
    } else if ([commandString isEqualToString:@"mute"]) {
        [playerController toggleMute];
    } else if ([commandString isEqualToString:@"volumeUp"]) {
        [playerController incrementVolume];
    } else if ([commandString isEqualToString:@"volumeDown"]) {
        [playerController decrementVolume];
    } else if ([commandString isEqualToString:@"moveMenuFocusUp"]) {
        [playerController navigateInInteractiveContent:VLC_PLAYER_NAV_UP];
    } else if ([commandString isEqualToString:@"moveMenuFocusDown"]) {
        [playerController navigateInInteractiveContent:VLC_PLAYER_NAV_DOWN];
    } else if ([commandString isEqualToString:@"moveMenuFocusLeft"]) {
        [playerController navigateInInteractiveContent:VLC_PLAYER_NAV_LEFT];
    } else if ([commandString isEqualToString:@"moveMenuFocusRight"]) {
        [playerController navigateInInteractiveContent:VLC_PLAYER_NAV_RIGHT];
    } else if ([commandString isEqualToString:@"menuFocusActivate"]) {
        [playerController navigateInInteractiveContent:VLC_PLAYER_NAV_ACTIVATE];
    } else if ([commandString isEqualToString:@"menuActivatePopupMenu"]) {
        [playerController navigateInInteractiveContent:VLC_PLAYER_NAV_POPUP];
    } else if ([commandString isEqualToString:@"menuActivateDiscRootMenu"]) {
        [playerController navigateInInteractiveContent:VLC_PLAYER_NAV_MENU];
    } else if ([commandString isEqualToString:@"stepForward"]) {
        if (parameterString) {
            int parameterInt = [parameterString intValue];
            switch (parameterInt) {
                case 1:
                    [playerController jumpForwardExtraShort];
                    break;
                case 3:
                    [playerController jumpForwardMedium];
                    break;
                case 4:
                    [playerController jumpForwardLong];
                    break;
                case 2:
                default:
                    [playerController jumpForwardShort];
                    break;
            }
        } else
            [playerController jumpForwardShort];
    } else if ([commandString isEqualToString:@"stepBackward"]) {
        //default: backwardShort
        if (parameterString) {
            int parameterInt = [parameterString intValue];
            switch (parameterInt) {
                case 1:
                    [playerController jumpBackwardExtraShort];
                    break;
                case 3:
                    [playerController jumpBackwardMedium];
                    break;
                case 4:
                    [playerController jumpBackwardLong];
                    break;
                case 2:
                default:
                    [playerController jumpBackwardShort];
                    break;
            }
        } else
            [playerController jumpBackwardShort];
    } else if ([commandString isEqualToString:@"incrementPlaybackRate"]) {
        [VLCMain.sharedInstance.playQueueController.playerController incrementPlaybackRate];
    } else if ([commandString isEqualToString:@"decrementPlaybackRate"]) {
        [VLCMain.sharedInstance.playQueueController.playerController decrementPlaybackRate];
    } else {
        msg_Err(getIntf(), "Unhandled AppleScript command '%s'", [commandString UTF8String]);
    }

    return nil;
}

@end

/*****************************************************************************
 * Category that adds AppleScript support to NSApplication
 *****************************************************************************/
@implementation NSApplication(ScriptSupport)

- (BOOL)scriptFullscreenMode
{
    return [VLCMain.sharedInstance.playQueueController.playerController fullscreen];
}

- (void)setScriptFullscreenMode:(BOOL)mode
{
    [VLCMain.sharedInstance.playQueueController.playerController setFullscreen:mode];
}

- (BOOL)muted
{
    return [VLCMain.sharedInstance.playQueueController.playerController mute];
}

- (BOOL)playing
{
    enum vlc_player_state playerState = [VLCMain.sharedInstance.playQueueController.playerController playerState];

    if (playerState == VLC_PLAYER_STATE_STARTED || playerState == VLC_PLAYER_STATE_PLAYING) {
        return YES;
    }

    return NO;
}

- (float)audioVolume
{
    return [VLCMain.sharedInstance.playQueueController.playerController volume];
}

- (void)setAudioVolume:(float)volume
{
    [VLCMain.sharedInstance.playQueueController.playerController setVolume:volume];
}

- (long long)audioDesync
{
    return MS_FROM_VLC_TICK([VLCMain.sharedInstance.playQueueController.playerController audioDelay]);
}

- (void)setAudioDesync:(long long)audioDelay
{
    [VLCMain.sharedInstance.playQueueController.playerController setAudioDelay: VLC_TICK_FROM_MS(audioDelay)];
}

- (int)currentTime
{
    return (int)SEC_FROM_VLC_TICK([VLCMain.sharedInstance.playQueueController.playerController time]);
}

- (void)setCurrentTime:(int)currentTime
{
    [VLCMain.sharedInstance.playQueueController.playerController setTimeFast: VLC_TICK_FROM_SEC(currentTime)];
}

- (float)playbackRate
{
    return VLCMain.sharedInstance.playQueueController.playerController.playbackRate;
}

- (void)setPlaybackRate:(float)playbackRate
{
    [VLCMain.sharedInstance.playQueueController.playerController setPlaybackRate:playbackRate];
}

- (NSInteger)durationOfCurrentItem
{
    return SEC_FROM_VLC_TICK(VLCMain.sharedInstance.playQueueController.playerController.durationOfCurrentMediaItem);
}

- (NSString *)pathOfCurrentItem
{
    return VLCMain.sharedInstance.playQueueController.playerController.URLOfCurrentMediaItem.path;
}

- (NSString *)nameOfCurrentItem
{
    return VLCMain.sharedInstance.playQueueController.playerController.nameOfCurrentMediaItem;
}

- (BOOL)playbackShowsMenu
{
    const struct vlc_player_title * const currentTitle = VLCMain.sharedInstance.playQueueController.playerController.selectedTitle;
    if (currentTitle == NULL) {
        return NO;
    }

    if (currentTitle->flags & VLC_PLAYER_TITLE_MENU) {
        return YES;
    }

    return NO;
}

- (BOOL)recordable
{
    return VLCMain.sharedInstance.playQueueController.playerController.recordable;
}

- (BOOL)recordingEnabled
{
    return VLCMain.sharedInstance.playQueueController.playerController.enableRecording;
}

- (void)setRecordingEnabled:(BOOL)recordingEnabled
{
    [VLCMain.sharedInstance.playQueueController.playerController setEnableRecording:recordingEnabled];
}

- (BOOL)shuffledPlayback
{
    const enum vlc_playlist_playback_order playbackOrder = VLCMain.sharedInstance.playQueueController.playbackOrder;
    return playbackOrder == VLC_PLAYLIST_PLAYBACK_ORDER_RANDOM ? YES : NO;
}

- (void)setShuffledPlayback:(BOOL)shuffledPlayback
{
    [VLCMain.sharedInstance.playQueueController setPlaybackOrder: shuffledPlayback ? VLC_PLAYLIST_PLAYBACK_ORDER_RANDOM : VLC_PLAYLIST_PLAYBACK_ORDER_NORMAL];
}

- (BOOL)repeatOne
{
    const enum vlc_playlist_playback_repeat repeatMode = VLCMain.sharedInstance.playQueueController.playbackRepeat;
    return repeatMode == VLC_PLAYLIST_PLAYBACK_REPEAT_CURRENT ? YES : NO;
}

- (void)setRepeatOne:(BOOL)repeatOne
{
    [VLCMain.sharedInstance.playQueueController setPlaybackRepeat: repeatOne == YES ? VLC_PLAYLIST_PLAYBACK_REPEAT_CURRENT : VLC_PLAYLIST_PLAYBACK_REPEAT_NONE];
}

- (BOOL)repeatAll
{
    const enum vlc_playlist_playback_repeat repeatMode = VLCMain.sharedInstance.playQueueController.playbackRepeat;
    return repeatMode == VLC_PLAYLIST_PLAYBACK_REPEAT_ALL ? YES : NO;
}

- (void)setRepeatAll:(BOOL)repeatAll
{
    [VLCMain.sharedInstance.playQueueController setPlaybackRepeat: repeatAll == YES ? VLC_PLAYLIST_PLAYBACK_REPEAT_ALL : VLC_PLAYLIST_PLAYBACK_REPEAT_NONE];
}

@end
