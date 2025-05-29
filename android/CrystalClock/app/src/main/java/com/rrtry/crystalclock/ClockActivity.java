package com.rrtry.crystalclock;

import android.app.NativeActivity;
import android.content.Context;
import android.os.Build;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.View;
import android.view.WindowManager;

public class ClockActivity extends NativeActivity {

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN              |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION         |
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY        |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN       |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION  |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P) {
            WindowManager.LayoutParams lp = getWindow().getAttributes();
            lp.layoutInDisplayCutoutMode  = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(lp);
        }
    }

    @SuppressWarnings("deprecation")
    public int getDisplayWidth() {

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
            return getWindowManager().getCurrentWindowMetrics().getBounds().width();

        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getRealMetrics(metrics);
        return metrics.widthPixels;
    }

    @SuppressWarnings("deprecation")
    public int getDisplayHeight() {

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
            return getWindowManager().getCurrentWindowMetrics().getBounds().height();

        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getRealMetrics(metrics);
        return metrics.heightPixels;
    }

    public int getTextSize(int dp) {
        return (int)pxFromDp(this, dp);
    }

    public static float pxFromDp(final Context context, final float dp) {
        return TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                dp,
                context.getResources().getDisplayMetrics()
        );
    }
}
