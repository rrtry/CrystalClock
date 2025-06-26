package com.rrtry.crystalclock;

import android.content.res.AssetManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.service.wallpaper.WallpaperService;
import android.view.Surface;
import android.view.SurfaceHolder;

public class RLWallpaperService extends WallpaperService {

    static {
        System.loadLibrary("crystalclock");
    }

    @Override
    public Engine onCreateEngine() {
        return new RLEngine();
    }

    private native void nativeInit(Surface surface, AssetManager assetManager, String internalDataPath);
    private native void nativeRender();
    private native void nativeShutdown();

    private class RLEngine extends WallpaperService.Engine {

        private static final String THREAD_NAME = "RenderThread";

        private static final int MSG_INIT     = 0;
        private static final int MSG_RENDER   = 1;
        private static final int MSG_PAUSE    = 2;
        private static final int MSG_RESUME   = 3;
        private static final int MSG_SHUTDOWN = 4;

        private static final int FRAMES_PER_SECOND = 60;
        private static final int DELAY_MILLIS      = (int)(1000.f / FRAMES_PER_SECOND);

        private HandlerThread renderThread;
        private Handler renderHandler;

        private volatile boolean isInitialized = false;
        private volatile boolean isRunning     = false;

        private Surface surface;
        private AssetManager assets;
        private String filesDir;

        @Override
        public void onCreate(SurfaceHolder surfaceHolder) {
            super.onCreate(surfaceHolder);
            if (isPreview()) return;
            renderThread = new HandlerThread(THREAD_NAME);
            renderThread.start();
            renderHandler = new Handler(renderThread.getLooper(), new Handler.Callback() {

                @Override
                public boolean handleMessage(Message msg) {
                    switch (msg.what) {
                        case MSG_INIT:
                            if (!isInitialized) {
                                nativeInit(surface, assets, filesDir);
                                isInitialized = true;
                            }
                            break;
                        case MSG_RENDER:
                            if (isInitialized && isRunning) {
                                nativeRender();
                                renderHandler.sendEmptyMessageDelayed(MSG_RENDER, DELAY_MILLIS);
                            }
                            break;
                        case MSG_PAUSE:
                            if (isRunning) {
                                isRunning = false;
                                renderHandler.removeMessages(MSG_RENDER);
                            }
                            break;
                        case MSG_RESUME:
                            if (!isRunning) {
                                isRunning = true;
                                renderHandler.sendEmptyMessage(MSG_RENDER);
                            }
                            break;
                        case MSG_SHUTDOWN:
                            if (isInitialized) {
                                renderHandler.removeMessages(MSG_RENDER);
                                nativeShutdown();
                                isInitialized = false;
                            }
                            break;
                        default:
                            return false;
                    }
                    return true;
                }
            });
        }

        @Override
        public void onSurfaceCreated(SurfaceHolder holder) {
            super.onSurfaceCreated(holder);
            if (isPreview()) return;
            surface  = holder.getSurface();
            assets   = getResources().getAssets();
            filesDir = getFilesDir().getAbsolutePath();
            renderHandler.sendEmptyMessage(MSG_INIT);
        }

        // TODO: implement
        @Override
        public void onSurfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            super.onSurfaceChanged(holder, format, width, height);
            if (isPreview()) return;
        }

        @Override
        public void onVisibilityChanged(boolean visible) {
            super.onVisibilityChanged(visible);
            if (isPreview()) return;
            if (visible)
                renderHandler.sendEmptyMessage(MSG_RESUME);
            else
                renderHandler.sendEmptyMessage(MSG_PAUSE);
        }

        @Override
        public void onSurfaceDestroyed(SurfaceHolder holder) {
            super.onSurfaceDestroyed(holder);
            if (isPreview()) return;
            renderHandler.sendEmptyMessage(MSG_SHUTDOWN);
        }

        @Override
        public void onDestroy() {
            super.onDestroy();
            if (isPreview()) return;
            renderThread.quitSafely();
        }
    }
}
