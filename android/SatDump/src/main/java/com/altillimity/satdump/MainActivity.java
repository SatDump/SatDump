package com.altillimity.satdump;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.content.Context;
import android.os.Environment;

import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.widget.Toast;
import android.content.Intent;

import androidx.core.content.PermissionChecker;

import android.app.Activity;
import android.net.Uri;

import androidx.loader.content.CursorLoader;

import android.database.Cursor;
import android.provider.MediaStore;
import android.content.pm.PackageManager;
import android.provider.MediaStore.MediaColumns;
import android.content.ContentResolver;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDeviceConnection;
import android.provider.DocumentsContract;

import com.altillimity.satdump.RealPathUtil;

import org.libsdl.app.SDLActivity;

import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.File;

import android.app.PendingIntent;
import android.content.IntentFilter;

import java.util.HashMap;

import android.content.BroadcastReceiver;

import java.util.Iterator;

import com.altillimity.satdump.SdrDevice;

/**
 * Original work by sf on 8/7/17.
 * https://github.com/sfalexrog/Imgui_Android
 */

public class MainActivity extends SDLActivity {

    /* A fancy way of getting the class name */
    private static final String TAG = MainActivity.class.getSimpleName();

    @Override
    protected String[] getLibraries() {
        return new String[]{"hidapi", "SDL2", "satdump_android"};
    }

    @Override
    protected String[] getArguments() {
        return new String[]{getFilesDir().getAbsolutePath()};
    }

    public native void initlibusb(int descriptor);

    // Thanks https://gist.github.com/tylerchesley/6198074
    public void copyFileOrDir(String path) {
        AssetManager assetManager = this.getAssets();
        String assets[] = null;
        try {
            assets = assetManager.list(path);
            if (assets.length == 0) {
                copyFile(path);
            } else {
                String fullPath = getFilesDir() + "/" + path;
                File dir = new File(fullPath);
                if (!dir.exists())
                    dir.mkdir();
                for (int i = 0; i < assets.length; ++i) {
                    copyFileOrDir(path + "/" + assets[i]);
                }
            }
        } catch (IOException ex) {
            Log.e(TAG, "I/O Exception", ex);
        }
    }

    private void copyFile(String filename) {
        AssetManager assetManager = this.getAssets();

        InputStream in = null;
        FileOutputStream out = null;
        try {
            in = assetManager.open(filename);
            String newFileName = getFilesDir() + "/" + filename;
            out = new FileOutputStream(newFileName);

            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            in.close();
            in = null;
            out.flush();
            out.close();
            out = null;
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        if (PermissionChecker.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
        if (PermissionChecker.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 1);

        /* We're going to unpack our assets to a location that's accessible
         * via the usual stdio means. This is to avoid using AAssetManager
         * in our native code. */
        Log.v(TAG, "Copying assets to accessible locations");

        SdrDevice.openSDRDevice(getApplicationContext());

        copyFileOrDir("resources");
        copyFileOrDir("pipelines");
        copyFileOrDir("Roboto-Medium.ttf");

        Log.v(TAG, getFilesDir().getAbsolutePath());
        Log.v(TAG, getExternalFilesDir(null).getAbsolutePath());
    }

    boolean waiting = false;
    String outputFile = "";
    public String getFilePath() {
        outputFile = "";

        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("*/*");
        intent.addCategory(Intent.CATEGORY_OPENABLE);

        try {
            startActivityForResult(Intent.createChooser(intent, "Select a File to process"), 0);
            waiting = true;
        } catch (android.content.ActivityNotFoundException ex) {
            Toast.makeText(this, "Please install a file manager.", Toast.LENGTH_SHORT).show();
        }

        try {while (waiting) Thread.sleep(100); } catch (InterruptedException ex) {}

        return outputFile;
    }

    boolean waiting1 = false;
    String outputFile1 = "";
    public String getFilePath1() {
        outputFile1 = "";

        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        //intent.setType("file/*"); 
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.addCategory(Intent.CATEGORY_DEFAULT);

        try {
            startActivityForResult(Intent.createChooser(intent, "Select an output directory"), 1);
            waiting1 = true;
        } catch (android.content.ActivityNotFoundException ex) {
            Toast.makeText(this, "Please install a file manager.", Toast.LENGTH_SHORT).show();
        }

        try {while (waiting1) Thread.sleep(100); } catch (InterruptedException ex) {}

        return outputFile1;
    }

    boolean waiting2 = false;
    String outputFile2 = "";
    public String getFilePath2() {
        outputFile1 = "";

        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("image/png2");
        intent.putExtra(Intent.EXTRA_TITLE, "projection.png");

        try {
            startActivityForResult(intent, 2);
            waiting2 = true;
        } catch (android.content.ActivityNotFoundException ex) {
            Toast.makeText(this, "Please install a file manager.", Toast.LENGTH_SHORT).show();
        }

        try {while (waiting2) Thread.sleep(100); } catch (InterruptedException ex) {}

        return outputFile2;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case 0:
                if (resultCode == RESULT_OK) {
                    outputFile = RealPathUtil.getRealPath(this, data.getData());
                    waiting = false;
                }
                break;
            case 1:
                if (resultCode == RESULT_OK) {
                    outputFile1 = RealPathUtil.getRealPath(this, DocumentsContract.buildDocumentUriUsingTree(data.getData(), DocumentsContract.getTreeDocumentId(data.getData())));
                    waiting1 = false;
                }
                break;
            case 2:
                if (resultCode == RESULT_OK) {
                    outputFile2 = RealPathUtil.getRealPath(this, data.getData());
                    waiting2 = false;
                }
                break;
        }
        super.onActivityResult(requestCode, resultCode, data);
    }
}
