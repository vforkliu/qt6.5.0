// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.browserfragment;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;

import org.chromium.base.ObserverList;
import org.chromium.browserfragment.interfaces.INavigationObserverDelegate;
import org.chromium.browserfragment.interfaces.INavigationParams;

/**
 * {@link NavigationObserverDelegate} notifies registered {@Link NavigationObserver}s of navigation
 * events in a Tab.
 */
class NavigationObserverDelegate extends INavigationObserverDelegate.Stub {
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    private ObserverList<NavigationObserver> mNavigationObservers =
            new ObserverList<NavigationObserver>();

    public NavigationObserverDelegate() {
        // Assert on UI thread as ObserverList can only be accessed from one thread.
        ThreadCheck.ensureOnUiThread();
    }

    /**
     * Registers a {@link NavigationObserver}. Call only from the UI thread.
     *
     * @return true if the observer was added to the list of observers.
     */
    boolean registerObserver(NavigationObserver tabObserver) {
        ThreadCheck.ensureOnUiThread();
        return mNavigationObservers.addObserver(tabObserver);
    }

    /**
     * Unregisters a {@link NavigationObserver}. Call only from the UI thread.
     *
     * @return true if the observer was removed from the list of observers.
     */
    boolean unregisterObserver(NavigationObserver tabObserver) {
        ThreadCheck.ensureOnUiThread();
        return mNavigationObservers.removeObserver(tabObserver);
    }

    @Override
    public void notifyNavigationStarted(@NonNull INavigationParams navigation) {
        mHandler.post(() -> {
            for (NavigationObserver observer : mNavigationObservers) {
                observer.onNavigationStarted(new Navigation(navigation));
            }
        });
    }

    @Override
    public void notifyNavigationCompleted(@NonNull INavigationParams navigation) {
        mHandler.post(() -> {
            for (NavigationObserver observer : mNavigationObservers) {
                observer.onNavigationCompleted(new Navigation(navigation));
            }
        });
    }

    @Override
    public void notifyNavigationFailed(@NonNull INavigationParams navigation) {
        mHandler.post(() -> {
            for (NavigationObserver observer : mNavigationObservers) {
                observer.onNavigationFailed(new Navigation(navigation));
            }
        });
    }

    @Override
    public void notifyLoadProgressChanged(double progress) {
        mHandler.post(() -> {
            for (NavigationObserver observer : mNavigationObservers) {
                observer.onLoadProgressChanged(progress);
            }
        });
    }
}