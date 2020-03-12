/*
 * Copyright (C) 2020 The MoKee Open Source Project
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

package org.mokee.device.parts;

import android.content.Context;
import android.media.AudioManager;
import android.view.KeyEvent;

import com.android.internal.os.DeviceKeyHandler;

public class KeyHandler implements DeviceKeyHandler {

    private static final String TAG = KeyHandler.class.getSimpleName();

    private static final int KEY_MUTE = 113;

    private final AudioManager mAudioManager;

    public KeyHandler(Context context) {
        mAudioManager = context.getSystemService(AudioManager.class);
    }

    public KeyEvent handleKeyEvent(KeyEvent event) {
        if (event.getScanCode() != KEY_MUTE) {
            return event;
        }

        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            mAudioManager.setRingerModeInternal(AudioManager.RINGER_MODE_SILENT);
        } else {
            mAudioManager.setRingerModeInternal(AudioManager.RINGER_MODE_NORMAL);
        }

        return null;
    }

}
