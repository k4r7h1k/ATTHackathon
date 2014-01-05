package com.example.pebblewand;

import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;

import java.util.*;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;


import com.att.m2x.M2X;
import com.att.m2x.Stream;
import com.att.m2x.StreamValue;
import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.PebbleKit.*;
import com.getpebble.android.kit.util.PebbleDictionary;
import com.google.common.collect.Iterators;

import de.tavendo.autobahn.WebSocketConnection;
import de.tavendo.autobahn.WebSocketException;
import de.tavendo.autobahn.WebSocketHandler;


public class MainActivity extends Activity {

	private final static UUID PEBBLE_APP_UUID = UUID.fromString("2daf4c14-973c-474f-80c3-706e1c6df62a");
	private boolean record=false;
	int bufferindices=0;
	final static int buffersize=30;
	final static int errorTolerance=3;
	//ArrayList<String> accBuffer=new ArrayList<String>();
	HashMap<Integer, String> accBuffer;
	final Handler handler = new Handler();
	 private final WebSocketConnection mConnection = new WebSocketConnection();
	 
	   private void start() {
	 
	      final String wsuri = "ws://sockets.mbed.org/ws/pebblewand/rw";
	 
	      try {
	         mConnection.connect(wsuri, new WebSocketHandler() {
	 
	            @Override
	            public void onOpen() {
	               Log.d("123", "Status: Connected to " + wsuri);
	               mConnection.sendTextMessage("Hello, world!");
	            }
	 
	            @Override
	            public void onTextMessage(String payload) {
	               Log.d("123", "Got echo: " + payload);
	            }
	 
	            @Override
	            public void onClose(int code, String reason) {
	               Log.d("123", "Connection lost.");
	            }
	         });
	      } catch (WebSocketException e) {
	 
	         Log.d("123", e.toString());
	      }
	   }
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		accBuffer=new HashMap<Integer, String>();
		start();
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		M2X.getInstance().setMasterKey(getString(R.string.m2x_master_key));
		boolean connected = PebbleKit.isWatchConnected(getApplicationContext());
		PebbleKit.startAppOnPebble(getApplicationContext(), PEBBLE_APP_UUID);
		Toast.makeText(getApplicationContext(),"Pebble is " + (connected ? "connected" : "not connected") , 10).show();
		Log.i(getLocalClassName(), "Pebble is " + (connected ? "connected" : "not connected"));
		if (PebbleKit.areAppMessagesSupported(getApplicationContext())) {
			  Log.i(getLocalClassName(), "App Message is supported!");
			  Toast.makeText(getApplicationContext(),"App Message is supported!" , 10).show();
			}
			else {
			  Log.i(getLocalClassName(), "App Message is not supported");
			  Toast.makeText(getApplicationContext(),"App Message is not supported!" , 10).show();
			}
		startService(new Intent(this, MyService.class));
		PebbleKit.registerReceivedDataHandler(this, new PebbleKit.PebbleDataReceiver(PEBBLE_APP_UUID) {
		    @Override
		    public void receiveData(final Context context, final int transactionId, final PebbleDictionary data) {
		      try{
		        	
		    	  	
	
		      handler.post(new Runnable() {
		        @Override
		        synchronized public void run() {
		        	Stream tmp=new Stream();
        			tmp.setName("amb-temp");
        			ArrayList<StreamValue> readings = new ArrayList<StreamValue>();
        			StreamValue temp=new StreamValue();
	        		temp.setValue(100000000);
		        	((TextView) findViewById(R.id.textView1)).setText(data.getString(0xabbababe));
		        	if(data.getString(0xabbababe).equals("First Item")){
		        		temp.setValue(100000000);
		        	}
		        	if(data.getString(0xabbababe).equals("Second Item")){
		        		temp.setValue(200000000);
		        	}
		        	try {
		        		readings.add(temp); 
		        		tmp.setValues(null, null, "1e93ba89b35bc96e32fac1c43cc1d9f5", readings, new Stream.BasicListener() {
	        			    public void onSuccess() {
	        			    }

	        			    public void onError(String errorMessage) {
	        			    }
	        			});
	        		} catch (Exception e) {
	        			e.printStackTrace();
	        		}
		        	//else ((TextView) findViewById(R.id.textView1)).setText("");
		        }
		      });
		      PebbleKit.sendAckToPebble(getApplicationContext(), transactionId);
		      }
		      catch(Exception e){
		    	  Log.i(getLocalClassName(), e.toString());
		      }
		    }

			
		});
		
		return true;
	
	}
	synchronized void analyzeBuffer(){
		int errorlevel=0;
		boolean signYChanged=false;
		boolean signZChanged=false;
		for(int i=1;i<20;i++)
		{
			
			String s[]=accBuffer.get(i-1).toString().split(" ");
			String s1[]=accBuffer.get(i).toString().split(" ");
			if(Integer.parseInt(s[1])<0 && Integer.parseInt(s1[1])>0 ){
				
				if(signYChanged==false)
					signYChanged=true;
				
			}
			if( Integer.parseInt(s[2])<0 && Integer.parseInt(s1[2])>0 ){
				if(signZChanged==false)
					signZChanged=true;
				
			}
		}
		if(signYChanged && signZChanged)
		{	Toast.makeText(getApplicationContext(), "Arm Twisted", 3).show();
			
			bufferindices=0;
		}
		
	}
	@Override
	protected void onDestroy(){
		super.onDestroy();
		
		PebbleKit.closeAppOnPebble(getApplicationContext(), PEBBLE_APP_UUID);
	}
}
