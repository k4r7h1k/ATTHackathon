package com.example.pebblewand;

import android.os.Bundle;
import android.os.Handler;

import java.util.*;
import android.app.Activity;
import android.content.Context;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.PebbleKit.*;
import com.getpebble.android.kit.util.PebbleDictionary;
import com.google.common.collect.Iterators;
public class CopyOfMainActivity extends Activity {

	private final static UUID PEBBLE_APP_UUID = UUID.fromString("4e845e4a-ce38-499c-942b-2f7da61d9fbc");
	private boolean record=false;
	int bufferindices=0;
	final static int buffersize=30;
	final static int errorTolerance=3;
	//ArrayList<String> accBuffer=new ArrayList<String>();
	HashMap<Integer, String> accBuffer;
	final Handler handler = new Handler();
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		accBuffer=new HashMap<Integer, String>();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		
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
		
		PebbleKit.registerReceivedDataHandler(this, new PebbleKit.PebbleDataReceiver(PEBBLE_APP_UUID) {
		    @Override
		    public void receiveData(final Context context, final int transactionId, final PebbleDictionary data) {
		      try{
		        	
		    	  	/*accBuffer.put(bufferindices, data.getString(0xabbababe));
		        	bufferindices++;
		        	if(bufferindices==20)
	        		{
	        			bufferindices=0;
	        			Log.i("Tag ",accBuffer.size()+" "+bufferindices);
	        			analyzeBuffer();
	        		}*/
		    	  analyzeString(data.getString(0xabbababe));
	
		      handler.post(new Runnable() {
		        @Override
		        synchronized public void run() {
		          
		        	((TextView) findViewById(R.id.textView1)).setText(data.getString(0xabbababe));
		        	try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
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
	void analyzeString(String s){
		String s1[]=s.split(" ");
		boolean signYChanged=false;
		boolean signZChanged=false;
		for(int i=1;i<10;i++)
		{
			if(Integer.parseInt(s1[(i-1)*3+1])<0 && Integer.parseInt(s1[(i)*3+1])>0 ){
				
				if(signYChanged==false)
					signYChanged=true;
				
			}
			if( Integer.parseInt(s1[(i-1)*3+2])<0 && Integer.parseInt(s1[(i)*3+2])>0 ){
				if(signZChanged==false)
					signZChanged=true;
				
			}
		}
		if(signYChanged && signZChanged)
		{	Toast.makeText(getApplicationContext(), "Arm Twisted", 3).show();
			
		
		}
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
