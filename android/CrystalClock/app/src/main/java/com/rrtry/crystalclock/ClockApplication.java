package com.rrtry.crystalclock;

import android.app.Application;
import android.content.res.AssetManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class ClockApplication extends Application {

    private static final String RESOURCES_DIR = "resources";
    private static final int BUFFER_SIZE = 1024;

    @Override
    public void onCreate() {
        super.onCreate();
        copyResources(getAssets(), getFilesDir());
    }

    private static void copyResources(AssetManager assets, File filesDir) {
        File resourcesDir = new File(filesDir.getAbsolutePath() + File.separator + RESOURCES_DIR);
        try {
            if (!resourcesDir.isDirectory()) {
                if (resourcesDir.mkdir())
                    copyResources(assets, filesDir.getAbsolutePath(), RESOURCES_DIR);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static void copy(AssetManager assets, String asset, File to) throws IOException {
        byte[] buffer = new byte[BUFFER_SIZE];
        try (InputStream in = assets.open(asset)) {
            try (OutputStream out = new FileOutputStream(to)) {
                while (in.read(buffer) > 0)
                    out.write(buffer);
            }
        }
    }

    private static void copyResources(AssetManager assets, String root, String path) throws IOException {

        String[] files = assets.list(path);
        if (files == null)
            return;

        String assetsPath;
        File filePath;

        for (String file : files) {

            assetsPath = path + File.separator + file;
            filePath   = new File(root + File.separator + assetsPath);
            files      = assets.list(assetsPath);

            if (files == null || files.length == 0) {
                copy(assets, assetsPath, filePath);
                continue;
            }
            if (filePath.mkdir())
                copyResources(assets, root, assetsPath);
        }
    }
}
