/* 

 * Copyright (c) 2012-2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.khronos.openvx.examples;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.text.DecimalFormat;
import java.util.StringTokenizer;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.CompoundButton.OnCheckedChangeListener;

import org.khronos.openvx.*;

/** @brief An example of how to call OpenVX from an Android app.
 * @author Erik Rainey <erik.rainey@gmail.com>
 */
public class OpenVXActivity extends Activity implements View.OnClickListener {

    /** Log tag ID */
    private static final String TAG = "OpenVXActivity";
    private Button mButton;

    /** OpenVX Types */
    private org.khronos.openvx.Context context;
    private org.khronos.openvx.Image input, output;
    private org.khronos.openvx.Buffer temp;
    private org.khronos.openvx.Scalar value;
    private org.khronos.openvx.Graph graph;
    private org.khronos.openvx.Node node;

    private int width;
    private int height;
    private int numUnits;

    /**
     * Constructor[s]
     */
    public OpenVXActivity() {
        Log.d(TAG, "OpenVXActivity running on :\n"+
                   "  MODEL: "+android.os.Build.MODEL+"\n"+
                   "RELEASE: "+android.os.Build.VERSION.RELEASE+"\n"+
                   " DEVICE: "+android.os.Build.DEVICE+"\n"+
                   "PRODUCT: "+android.os.Build.PRODUCT+"\n"+
                   "  BOARD: "+android.os.Build.BOARD+"\n");
    }

    public boolean SetupGraph() {
        // load the extension
        context = new org.khronos.openvx.Context();
        if (context.loadKernels("xyz") == true) {
            Log.d(TAG, "There are "+context.getNumKernels()+" kernels available\n");
            Log.d(TAG, "There are "+context.getNumModules()+" modules loaded\n");
            Log.d(TAG, "There are "+context.getNumReferences()+" references declared\n");
            Log.d(TAG, "There are "+context.getNumTargets()+" targets available\n");
            for (int t = 0; t < context.getNumTargets(); t++)
            {
                Target target = context.getTarget(t);
                Log.d(TAG, "Target "+target.getName()+" has "+target.getNumKernels()+" kernels\n");
                Target.TargetKernel[] tk = target.getTable();
                if (tk != null)
                {
                    for (int k = 0; k < tk.length; k++)
                    {
                        Log.d(TAG, "\tkernel["+tk[k].enumeration+"] "+tk[k].name+"\n");
                    }
                }
            }
            width = 320;
            height = 240;
            numUnits = 374;
            input = context.createImage(width, height, Image.VX_DF_IMAGE_Y800);
            output = context.createImage(width, height, Image.VX_DF_IMAGE_Y800);
            temp = context.createBuffer(4, numUnits); // 4 == sizeof(int)
            value = context.createScalar((int)2);

            // create the graph
            graph = context.createGraph();

            // create the node
            node = XYZNode(graph, input, temp, output, value);

            // verify the correctness of the graph...
            if (graph.verify() == Status.SUCCESS)
            {
                // get copy of the image data
                byte[] data = new byte[input.computePatchSize(0, 0, width, height, 0)];
                if (input.getPatch(0, 0, width, height, 0, data) == Status.SUCCESS) {
                    for (int i = 0; i < width*height; i++)
                        data[i] = 42;
                    input.setPatch(0, 0, width, height, 0, data);
                    Log.d(TAG, "Initialized input image!\n");
                }
                // get a copy of the buffer data to "set it"
                int[] buf = new int[numUnits];
                if (temp.getRange(0, numUnits, buf) == Status.SUCCESS) {
                    for (int i = 0; i < numUnits; i++)
                        buf[i] = 97;
                    temp.setRange(0, numUnits, buf);
                    Log.d(TAG, "Initialized temp buffer!\n");
                }
                else
                    Log.e(TAG, "Failed to get temp buffer range\n");
                return true;
            }
            else
                return false;
        } else
            return false;
    }

    public boolean TeardownGraph() {
        // destroy everything
        graph.destroy();

        input.destroy();
        output.destroy();
        temp.destroy();
        value.destroy();

        context.destroy();

        return true;
    }

    private Node XYZNode(Graph graph, Image in, Buffer temp, Image out, Scalar value) {
        Kernel kernel = context.getKernel("org.khronos.example.xyz");
        if (kernel != null) {
            Node node = graph.createNode(kernel);
            if (node != null) {
                node.setParameter(0, Kernel.INPUT, input);
                node.setParameter(1, Kernel.INPUT, value);
                node.setParameter(2, Kernel.OUTPUT, output);
                node.setParameter(3, Kernel.BIDIRECTIONAL, temp);
            }
            kernel.release();
        }
        return node;
    }


    public void onClick(View view) {
        int status = graph.process();
        if (status == Status.SUCCESS) {
            Log.d(TAG, "Processed Graph Succesfully!\n");
        } else {
            Log.e(TAG, "Failed to process Graph! (status="+status+")\n");
        }
    }

    /** Called with the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Inflate our UI from its XML layout description.
        setContentView(R.layout.skeleton_activity);

        mButton = (Button) findViewById(R.id.button);
        mButton.setOnClickListener(this);
    }

    /* (non-Javadoc)
     * @see android.app.Activity#onConfigurationChanged(android.content.res.Configuration)
     */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    /* (non-Javadoc)
     * @see android.app.Activity#onSaveInstanceState(android.os.Bundle)
     */
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
    }

    /* (non-Javadoc)
     * @see android.app.Activity#onRestoreInstanceState(android.os.Bundle)
     */
    @Override
    protected void onRestoreInstanceState(Bundle inState) {
        super.onRestoreInstanceState(inState);
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    /** Called when the activity is about to start interacting with the user. */
    @Override
    protected void onResume() {
        super.onResume();
        SetupGraph();
    }

    /** Called when the activity is about to resume a previous activity */
    @Override
    protected void onPause() {
        super.onPause();
        TeardownGraph();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    /**
     * Called when your activity's options menu needs to be created.
     */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.app_menu, menu);
        return true;
    }

    /**
     * Called right before your activity's option menu is displayed.
     */
    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);

        // Before showing the menu, we need to decide whether the clear
        // item is enabled depending on whether there is text to clear.
        //menu.findItem(ABOUT_MENU_ID).setVisible(mEditor.getText().length() > 0);

        return true;
    }

    /**
     * Called when a menu item is selected.
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.menu_copyright:
                showAboutMenu();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    /**
     * Shows some information about this application and version
     */
    private void showAboutMenu() {
        PackageInfo myPI;
        String codeVersion = "";
        try {
            myPI = getPackageManager().getPackageInfo(getPackageName(),0);
            codeVersion = "\n"+getString(R.string.about_box_text)+"\n\n"+
                          "Version : " + myPI.versionCode+"  "+ myPI.versionName;
        } catch (NameNotFoundException e) {
            Log.e(TAG, "Could not find about box text:"+ e.toString());
        }
        AlertDialog.Builder infoDialogBuilder = new AlertDialog.Builder(this);
        infoDialogBuilder.setMessage(codeVersion);
        infoDialogBuilder.setTitle("Copyright");
        infoDialogBuilder.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                /* User clicked OK */
            }
        });
        AlertDialog infoDialog = infoDialogBuilder.create();
        infoDialog.show();
    }
}

