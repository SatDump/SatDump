package imgui.example.android

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

import kotlinx.coroutines.sync.Mutex;

import android.Manifest;
import androidx.core.content.PermissionChecker;
import androidx.core.app.ActivityCompat;
import android.content.pm.PackageManager;
import android.provider.DocumentsContract;

// Extension on intent
fun Intent?.getFilePath(context: Context): String {
    return this?.data?.let { data -> RealPathUtil.getRealPath(context, data) ?: "" } ?: ""
}

// Extension on intent
fun Intent?.getFilePathDir(context: Context): String {
    return this?.data?.let { data -> RealPathUtil.getRealPath(context, DocumentsContract.buildDocumentUriUsingTree(data, DocumentsContract.getTreeDocumentId(data))) ?: "" } ?: ""
}

fun Uri?.getFilePath(context: Context): String {
    return this?.let { uri -> RealPathUtil.getRealPath(context, uri) ?: "" } ?: ""
}

class MainActivity : NativeActivity() {
    private val TAG : String = "SatDump";

    fun checkAndAsk(permission: String) {
        if (PermissionChecker.checkSelfPermission(this, permission) != PermissionChecker.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(permission), 1);
        }
    }

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Ask for required permissions, without these the app cannot run.
        checkAndAsk(Manifest.permission.WRITE_EXTERNAL_STORAGE);
        checkAndAsk(Manifest.permission.READ_EXTERNAL_STORAGE);
        checkAndAsk(Manifest.permission.INTERNET);
    }


    public fun getAppDir(): String {
        val fdir = getFilesDir().getAbsolutePath();

        // Extract all resources to the app directory
        val aman = getAssets();
        extractDir(aman, fdir + "/pipelines", "pipelines");
        extractDir(aman, fdir + "/resources", "resources");
        extractDir(aman, fdir + "/plugins", "plugins");
        extractFile(aman, fdir + "/satdump_cfg.json", "satdump_cfg.json");
        extractFile(aman, fdir + "/Roboto-Medium.ttf", "Roboto-Medium.ttf");
        //createIfDoesntExist(fdir + "/plugins");

        return fdir;
    }

    fun showSoftInput() {
        val inputMethodManager = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
        inputMethodManager.showSoftInput(this.window.decorView, 0)
    }

    fun hideSoftInput() {
        val inputMethodManager = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
        inputMethodManager.hideSoftInputFromWindow(this.window.decorView.windowToken, 0)
    }

    // Queue for the Unicode characters to be polled from native code (via pollUnicodeChar())
    private var unicodeCharacterQueue: LinkedBlockingQueue<Int> = LinkedBlockingQueue()

    // We assume dispatchKeyEvent() of the NativeActivity is actually called for every
    // KeyEvent and not consumed by any View before it reaches here
    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        if (event.action == KeyEvent.ACTION_DOWN) {
            unicodeCharacterQueue.offer(event.getUnicodeChar(event.metaState))
        }
        return super.dispatchKeyEvent(event)
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


    var selectFile_Input_done : AtomicBoolean = AtomicBoolean(false);
    var selectFile_Input_resu : String = "";
    public fun selectFile_Input() : String {
        var file_intent = Intent(Intent.ACTION_GET_CONTENT);
        file_intent.setType("*/*");
        file_intent.addCategory(Intent.CATEGORY_OPENABLE);
        val final_intent = Intent.createChooser(file_intent, "Select stuff");
        selectFile_Input_done.set(false);
        startActivityForResult(final_intent, 1);
        //while(!selectFile_Input_done.get());
        return selectFile_Input_resu;
    }

    public fun selectFile_Input_get() : String {
        var tmp = selectFile_Input_resu;
        selectFile_Input_resu = "";
        return tmp;
    }

    var selectFile_Output_done : AtomicBoolean = AtomicBoolean(false);
    var selectFile_Output_resu : String = "";
    public fun selectFile_Output() : String {
        var file_intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        //file_intent.setType("*/*");
        file_intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        file_intent.addCategory(Intent.CATEGORY_DEFAULT);
        val final_intent = Intent.createChooser(file_intent, "Select stuff");
        selectFile_Output_done.set(false);
        startActivityForResult(final_intent, 2);
        //while(!selectFile_Input_done.get());
        return selectFile_Output_resu;
    }

    public fun selectFile_Output_get() : String {
        var tmp = selectFile_Output_resu;
        selectFile_Output_resu = "";
        return tmp;
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data);

        if (resultCode == RESULT_OK && requestCode == 1) {
            Log.e(TAG, "spdlog Got URI " + data);
            val fpath = data.getFilePath(getApplicationContext()); //RealPathUtil.getRealPath(getApplicationContext(), Uri.parse(data?.data?.path)); //?.data?.path;
            Log.e(TAG, "spdlog Got URI " + fpath);
            selectFile_Input_resu = fpath;
            selectFile_Input_done.set(true);
        }

        if (resultCode == RESULT_OK && requestCode == 2) {
            Log.e(TAG, "spdlog Got URI " + data);
            val fpath = data.getFilePathDir(getApplicationContext()); //RealPathUtil.getRealPath(getApplicationContext(), Uri.parse(data?.data?.path)); //?.data?.path;
            Log.e(TAG, "spdlog Got URI " + fpath);
            selectFile_Output_resu = fpath;
            selectFile_Output_done.set(true);
        }
    }
}
