package com.altillimity.satdump

import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Build
import android.app.NativeActivity
import android.os.Bundle
import android.content.Context
import android.view.inputmethod.InputMethodManager
import android.view.KeyEvent
import java.util.concurrent.LinkedBlockingQueue
import android.util.Log
import android.content.res.AssetManager
import java.io.*
import java.util.concurrent.atomic.AtomicBoolean

import android.content.Intent;
import android.app.Activity;
import android.net.Uri;

import RealPathUtil;

import android.Manifest;
import androidx.core.content.PermissionChecker;
import androidx.core.app.ActivityCompat;
import android.content.pm.PackageManager;
import android.provider.DocumentsContract;

import android.content.BroadcastReceiver;
import android.app.PendingIntent;
import android.content.IntentFilter;

import android.view.View;
import android.view.ViewGroup;
import android.view.Window;

import android.widget.RelativeLayout;
import android.widget.EditText;
import android.text.TextWatcher;
import android.text.Editable;
import android.text.InputType;

import android.view.WindowManager;

import org.woheller69.freeDroidWarn.FreeDroidWarn;
import org.satdump.SatDump.BuildConfig;

// Extension on intent
fun Intent?.getFilePath(context: Context): String {
    return this?.data?.let { data -> RealPathUtil.getRealPath(context, data) ?: "" } ?: ""
}

// Extension on intent
fun Intent?.getFilePathDir(context: Context): String {
    return this?.data?.let { data -> RealPathUtil.getRealPath(context, DocumentsContract.buildDocumentUriUsingTree(data, DocumentsContract.getTreeDocumentId(data))) ?: "" } ?: ""
}

class MainActivity : NativeActivity(), TextWatcher {
    private val TAG : String = "SatDump";

    fun checkAndAsk(permission: String) {
        if (PermissionChecker.checkSelfPermission(this, permission) != PermissionChecker.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(permission), 1);
        }
    }

    private val ACTION_USB_PERMISSION = "org.satdump.SatDump.USB_PERMISSION"
    private val usbLock = Any()
    // 0 = waiting, 1 = granted, -1 = denied, -2 = detached, -3 = request failed.
    private val usbResults = mutableMapOf<String, Int>()
    private val usbManager: UsbManager
        get() = getSystemService(Context.USB_SERVICE) as UsbManager

    // Called from native code. Only the dialog request is dispatched to the UI thread.
    fun rtlUsbPermission(path: String, request: Boolean): Int = synchronized(usbLock) {
        val device = usbManager.deviceList[path] ?: return@synchronized -2
        if (usbManager.hasPermission(device)) return@synchronized 1
        if (!request) return@synchronized usbResults[path] ?: -3
        if (usbResults[path] == 0) return@synchronized 0
        usbResults[path] = 0
        runOnUiThread {
            synchronized(usbLock) {
                val current = usbManager.deviceList[path]
                if (current == null) {
                    usbResults[path] = -2
                } else if (usbManager.hasPermission(current)) {
                    usbResults[path] = 1
                } else {
                    try {
                        // Mutable so UsbManager can supply EXTRA_DEVICE/PERMISSION_GRANTED.
                        // Package-scoping prevents other apps from receiving this callback.
                        val flags = PendingIntent.FLAG_UPDATE_CURRENT or
                            if (Build.VERSION.SDK_INT >= 31) PendingIntent.FLAG_MUTABLE else 0
                        val reply = Intent(ACTION_USB_PERMISSION).setPackage(packageName)
                            .setData(Uri.parse("satdump-usb:" + Uri.encode(path)))
                        val pending = PendingIntent.getBroadcast(this, 0, reply, flags)
                        usbManager.requestPermission(current, pending)
                    } catch (e: RuntimeException) {
                        usbResults[path] = -3
                        Log.e(TAG, "Could not request USB permission for RTL-SDR", e)
                    }
                }
            }
        }
        0
    }

    private val usbReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val device = intent.getParcelableExtra<UsbDevice>(UsbManager.EXTRA_DEVICE) ?: return
            synchronized(usbLock) {
                when (intent.action) {
                    ACTION_USB_PERMISSION -> {
                        if (usbResults[device.deviceName] != 0) return
                        val granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                            && usbManager.hasPermission(device)
                        usbResults[device.deviceName] = if (granted) 1 else -1
                        if (granted) Log.i(TAG, "USB permission for RTL-SDR granted")
                        else Log.w(TAG, "USB permission for RTL-SDR was denied")
                    }
                    UsbManager.ACTION_USB_DEVICE_DETACHED -> {
                        usbResults[device.deviceName] = -2
                        Log.i(TAG, "USB device detached: " + device.deviceName)
                    }
                    UsbManager.ACTION_USB_DEVICE_ATTACHED -> usbResults.remove(device.deviceName)
                    else -> Unit
                }
            }
        }
    }

    override fun onDestroy() {
        unregisterReceiver(usbReceiver)
        super.onDestroy()
    }

    public var mLayout : ViewGroup? = null;
    public var editText : EditText? = null;
    public var lastFiller : String? = null;

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Ask for required permissions, without these the app cannot run.
        checkAndAsk(Manifest.permission.WRITE_EXTERNAL_STORAGE);
        checkAndAsk(Manifest.permission.READ_EXTERNAL_STORAGE);
        checkAndAsk(Manifest.permission.INTERNET);

        val filter = IntentFilter(ACTION_USB_PERMISSION)
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        if (Build.VERSION.SDK_INT >= 33) registerReceiver(usbReceiver, filter, Context.RECEIVER_NOT_EXPORTED)
        else registerReceiver(usbReceiver, filter)

        // Hide system bars
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);

        // Keep screen on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Text crap
        mLayout = RelativeLayout(this);
        editText = EditText(this.applicationContext!!);
        mLayout!!.addView(editText, RelativeLayout.LayoutParams(10000, 10000));
        editText!!.setVisibility(View.VISIBLE);
        editText!!.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        editText!!.requestFocus();
        editText!!.setText(" ");
        editText!!.setSelection(1);
        lastFiller = " ";
        editText!!.addTextChangedListener(this);

        setContentView(mLayout);

        FreeDroidWarn.showWarningOnUpgrade(this, BuildConfig.VERSION_CODE);
    }


    public fun getAppDir(): String {
        val fdir = getFilesDir().getAbsolutePath();

        // Extract all resources to the app directory
        val aman = getAssets();
        extractDir(aman, fdir + "/resources", "resources");
        // extractDir(aman, fdir + "/plugins", "plugins");
        extractFile(aman, fdir + "/satdump_cfg.json", "satdump_cfg.json");
        //createIfDoesntExist(fdir + "/plugins");

        return fdir;
    }

    public fun get_plugins_directory() : String {
        return getApplicationInfo().nativeLibraryDir;
    }

    public fun get_dpi() : Float {
        return getResources().getDisplayMetrics().density;
    }

    fun showSoftInput() {
        runOnUiThread {
            val inputMethodManager = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
            inputMethodManager.showSoftInput(editText, 0)
        }
    }

    fun hideSoftInput() {
        runOnUiThread {
            val inputMethodManager = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
            inputMethodManager.hideSoftInputFromWindow(editText!!.windowToken, 0)
        }
    }

    // Queue for the Unicode characters to be polled from native code (via pollUnicodeChar())
    private var unicodeCharacterQueue: LinkedBlockingQueue<Int> = LinkedBlockingQueue()

    // Not all Android keyboard trigger a KeyEvent
    // so I had to get around it somehow...
    // I'm not super proud of this and it has downsides,
    // but at least it works!
    // Hecking Android not having a simple function
    // to get Key events......... WHY!?
    override fun afterTextChanged(s : Editable) {
        if(!editText!!.getText().toString().startsWith(" "))
        {
            editText!!.setText(" ");
            editText!!.setSelection(1);
        }

        lastFiller = editText!!.getText().toString();
    }

    override fun beforeTextChanged(s : CharSequence, start: Int, count: Int, after: Int) {
    }

    override fun onTextChanged(s : CharSequence, start: Int, before: Int, count: Int) {
        if(editText!!.getText().toString() != lastFiller) {
            if(before < count) {
                var char2 = s.get(s.length - 1);
                unicodeCharacterQueue.offer(char2.toInt());
            } else if(before > count) {
                unicodeCharacterQueue.offer(8); // BackSpace
            }
        }
    }

    fun pollUnicodeChar(): Int {
        return unicodeCharacterQueue.poll() ?: 0
    }

    public fun extractFile(aman: AssetManager, local: String, rsrc: String): Int {
        val lpath = local;
        val rpath = rsrc;

        Log.w(TAG, "Extracting '" + rpath + "' to '" + lpath + "'");

        // This is a file, extract it
        val _os = FileOutputStream(lpath);
        val _is = aman.open(rpath);
        val ilen = _is.available();
        var fbuf = ByteArray(ilen);
        _is.read(fbuf, 0, ilen);
        _os.write(fbuf);
        _os.close();
        _is.close();

        return 0;
    }

    public fun extractDir(aman: AssetManager, local: String, rsrc: String): Int {
        val flist = aman.list(rsrc);
        var ecount = 0;
        for (fp in flist!!) {
            val lpath = local + "/" + fp;
            val rpath = rsrc + "/" + fp;

            Log.w(TAG, "Extracting '" + rpath + "' to '" + lpath + "'");

            // Create local path if non-existent
            createIfDoesntExist(local);
            
            // Create if directory
            val ext = extractDir(aman, lpath, rpath);

            // Extract if file
            if (ext == 0) {
                // This is a file, extract it
                val _os = FileOutputStream(lpath);
                val _is = aman.open(rpath);
                val ilen = _is.available();
                var fbuf = ByteArray(ilen);
                _is.read(fbuf, 0, ilen);
                _os.write(fbuf);
                _os.close();
                _is.close();
            }

            ecount++;
        }
        return ecount;
    }

    public fun createIfDoesntExist(path: String) {
        // This is a directory, create it in the filesystem
        var folder = File(path);
        var success = true;
        if (!folder.exists()) {
            success = folder.mkdirs();
        }
        if (!success) {
            Log.e(TAG, "Could not create folder with path " + path);
        }
    }

    // Handle selecting a file
    var select_file_result : String = "";
    public fun select_file() {
        var file_intent = Intent(Intent.ACTION_GET_CONTENT);
        file_intent.setType("*/*");
        file_intent.addCategory(Intent.CATEGORY_OPENABLE);
        val final_intent = Intent.createChooser(file_intent, "Select File");
        startActivityForResult(final_intent, 1);
    }

    public fun select_file_get() : String {
        var tmp = select_file_result;
        select_file_result = "";
        return tmp;
    }

    // Handle selecting a directory
    var select_directory_result : String = "";
    public fun select_directory() {
        var file_intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        file_intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        file_intent.addCategory(Intent.CATEGORY_DEFAULT);
        val final_intent = Intent.createChooser(file_intent, "Select Directory");
        startActivityForResult(final_intent, 2);
    }

    public fun select_directory_get() : String {
        var tmp = select_directory_result;
        select_directory_result = "";
        return tmp;
    }

    // Handle savinf a file
    var select_filesave_result : String = "";
    public fun select_filesave(name: String) {
        var file_intent = Intent(Intent.ACTION_CREATE_DOCUMENT);
        file_intent.setType("*/*");
        file_intent.putExtra(Intent.EXTRA_TITLE, name);
        val final_intent = Intent.createChooser(file_intent, "Select Destination File");
        startActivityForResult(final_intent, 3);
    }

    public fun select_filesave_get() : String {
        var tmp = select_filesave_result;
        select_filesave_result = "";
        return tmp;
    }

    public fun openURL(url: String) {
        val browserIntent = Intent(Intent.ACTION_VIEW, Uri.parse(url));
        startActivity(browserIntent);
    }

    // Receive results of the above
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data);

        try {
            if (requestCode == 1) {
                if(resultCode == RESULT_OK)
                    select_file_result = data.getFilePath(getApplicationContext());
                else if(resultCode == RESULT_CANCELED)
                    select_file_result = "NO_PATH_SELECTED";
            }

            if (requestCode == 2) {
                if(resultCode == RESULT_OK)
                    select_directory_result = data.getFilePathDir(getApplicationContext());
                else if(resultCode == RESULT_CANCELED)
                    select_directory_result = "NO_PATH_SELECTED";
            }

            if (requestCode == 3) {
                if(resultCode == RESULT_OK)
                    select_filesave_result = data.getFilePath(getApplicationContext());
                else if(resultCode == RESULT_CANCELED)
                    select_filesave_result = "NO_PATH_SELECTED";
            } 
        } catch (e: java.lang.RuntimeException) {
            Log.w(TAG, "Error! " + e.message);
        }
    }
}
