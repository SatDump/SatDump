package com.altillimity.satdump;

import android.util.Log;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import java.util.Set;
import java.util.HashSet;
import android.content.Context;
import java.util.HashMap;
import android.util.Pair;
import java.util.Map.Entry;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.hardware.usb.UsbDeviceConnection;

/*
    All credits go to https://github.com/martinmarinov/rtl_tcp_andro-
    This code was adapted from there, as the goal is a bit different.

    Currently only handles RTL-SDR, and definitely VERY rough...
    I will probably rewrite quite a bit later on, when adding support
    for other devices.
*/

public class SdrDevice {

    private static final String TAG = SdrDevice.class.getSimpleName();

    public static UsbDevice usbDevice = null;

    private static Boolean granted = false;

    public static UsbDeviceConnection connection = null;

    public static void openSDRDevice(final Context ctx) {
        Log.e(TAG, "Opening SDR Device...");

        Set<UsbDevice> devices = getAvailableUsbDevices(ctx);

        for(UsbDevice dev : devices) {
            Log.e(TAG, dev.getDeviceName());
            usbDevice = dev;
        }

        granted = false;

        if(devices.size() <= 0) {
            return;
        }

        UsbManager manager = (UsbManager) ctx.getSystemService(Context.USB_SERVICE);
        manager.requestPermission(usbDevice, PendingIntent.getBroadcast(ctx, 0, new Intent("com.android.example.USB_PERMISSION"), 0));

        ctx.registerReceiver(new BroadcastReceiver() {
			@Override
			public void onReceive(Context context, Intent intent) {
				String action = intent.getAction();
				if ("com.android.example.USB_PERMISSION".equals(action)) {
					synchronized (this) {
						UsbManager manager = (UsbManager) ctx.getSystemService(Context.USB_SERVICE);
						UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
						if (device.equals(usbDevice)) {
							if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
								if (!manager.hasPermission(device)) {
                                    Log.e(TAG,"Permissions were granted but can't access the device");
									//task.setDone(null);
                                   
								} else {
                                    Log.e(TAG,"Permissions granted and device is accessible");
									//task.setDone(manager.openDevice(device));
                                    granted = true;

                                    connection = manager.openDevice(usbDevice);

                                    if(usbDevice.getVendorId() == 3034)
                                        doOpenRTL(connection.getFileDescriptor(), usbDevice.getDeviceName());
                                    else if(usbDevice.getVendorId() == 7504)
                                        doOpenAirspy(connection.getFileDescriptor(), usbDevice.getDeviceName());
								}
							} else {
                                Log.e(TAG,"Extra permission was not granted");
								//task.setDone(null);
                               
							}
							context.unregisterReceiver(this);
						} else {
                            Log.e(TAG, "Got a permission for an unexpected device %s. Expected %s.");
							//task.setDone(null);
                            
						}
					}
				} else {
                    Log.e(TAG,"Unexpected action");
					//task.setDone(null);
                  
				}
			}
		}, new IntentFilter("com.android.example.USB_PERMISSION"));
    }

    /** This method is safe to be called from old Android versions */
    public static Set<UsbDevice> getAvailableUsbDevices(final Context ctx) {
        Set<UsbDevice> usbDevices = new HashSet<>();
        if (true) {
            final UsbManager manager = (UsbManager) ctx.getSystemService(Context.USB_SERVICE);

            HashSet<Pair<Integer, Integer>> allowed = new HashSet<Pair<Integer, Integer>>();;// = {{3034, 10296}};
            allowed.add(new Pair<Integer, Integer>(3034, 10296)); // RTL-SDR
            allowed.add(new Pair<Integer, Integer>(7504, 24737)); // Airspy
            HashMap<String, UsbDevice> deviceList = manager.getDeviceList();

            for (final Entry<String, UsbDevice> desc : deviceList.entrySet()) {
                UsbDevice candidate = desc.getValue();
                final Pair<Integer, Integer> candidatePair = new Pair<>(candidate.getVendorId(), candidate.getProductId());
                if (allowed.contains(candidatePair)) usbDevices.add(candidate);
            }
        }
        return usbDevices;
    }

    private static native boolean doOpenRTL(int fd, String path);
    private static native boolean doOpenAirspy(int fd, String path);
}
