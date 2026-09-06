package com.normaker.nativefull;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.media.AudioAttributes;
import android.media.SoundPool;
import java.util.HashMap;
import java.util.Map;
import android.content.Intent;
import android.net.Uri;
import android.webkit.ValueCallback;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.webkit.RenderProcessGoneDetail;
import android.webkit.JavascriptInterface;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebChromeClient;
import android.webkit.WebChromeClient.FileChooserParams;
import android.util.Base64;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.ByteArrayOutputStream;

public final class MainActivity extends Activity {
    static {
        System.loadLibrary("gm82_android");
    }

    private WebView webView;
    private static final int REQUEST_GMX_GMZ = 8201;
    private static final int REQUEST_FILE_CHOOSER = 8202;
    private ValueCallback<Uri[]> pendingFileCallback;
    private SoundPool soundPool;
    private final Map<Integer, Integer> soundResources = new HashMap<>();
    private final Map<Integer, Integer> soundStreams = new HashMap<>();

    private static native boolean nativeRuntimeCreate(int width, int height);
    private static native void nativeRuntimeDestroy();
    private static native void nativeRuntimeStep(float delta);
    private static native void nativeRuntimeKey(int keyCode, boolean down);
    private static native void nativeRuntimeSetRoom(int roomId, int width, int height, boolean clearInstances);
    private static native int nativeRuntimeGetRoom();
    private static native String nativeRuntimeSnapshotJson();
    private static native String nativeEvaluateGml(String source);
    private static native boolean nativeRuntimeRegisterEvent(int objectId, int mainType, int subType, String source);
    private static native boolean nativeRuntimeRegisterObject(int objectId, String name);
    private static native boolean nativeRuntimeRegisterScript(String name, String source);
    private static native void nativeRuntimeClearScripts();
    private static native void nativeRuntimeClearEvents();
    private static native int nativeRuntimeAddInstance(int objectId, int spriteId, int spriteWidth, int spriteHeight, int spriteSubimages, float x, float y, float vx, float vy);
    private static native boolean nativeRuntimeSetSpriteBitmap(int spriteId, int frame, int width, int height, byte[] rgba);
    private static native boolean nativeRuntimeRenderBitmap(Bitmap target);
    private static native void nativeRuntimeClearInstances();
    private static native void nativeRuntimeClearRoomTransient();
    private static native boolean nativeRuntimeExecuteGml(int instanceId, String source);
    private static native String nativeRuntimeConsumeSoundCommands();
    private static native boolean nativeExportNorJson(String json, String outputPath);
    private static native int nativeImportGmxGmz(String path, String outputDir);
    private static native int nativeExportGmxGmz(String sourceDir, String outputPath, String kind);
    private static native boolean nativeExportGmkRaw(String sourcePath, String outputPath);
    private static native boolean nativeExportGmxSemantic(String sourceDir, String outputDir, String projectName);
    private static native String nativeCoreIdentity();
    private static native boolean nativeValidateGmk(byte[] bytes);
    private static native String nativeGmkHeaderJson(byte[] bytes);
    private static native String nativeGmkLayoutJson(byte[] bytes);
    private static native String nativeGmkChunkInventory(byte[] bytes);
    private static native String nativeGmkResourceManifest(byte[] bytes);
    private static native String nativeImportGmkSnapshot(byte[] bytes, String outputDir);
    private static native int nativeCompileGml(String source);
    private static native boolean nativeCodeExists(int codeId);
    private static native int nativeCodeGetArgCount(int codeId);
    private static native void nativeCodeDestroy(int codeId);
    private static native int nativeCodeExecute(int instanceId, int codeId);
    private static native boolean nativeExportRom(String title, String outputPath, int kind);
    private static native int nativeDetectRom(String path);
    private static native boolean nativeValidateRom(String path, int kind);
    private static native void nativeClearResourceRegistry();
    private static native int nativeRegisterResource(int kind, int id, String name, int width, int height, int frames);
    private static native int nativeResourceCount();
    private static native int nativeRegisterObjectEvent(int objectId, int mainType, int subType, String source);
    private static native int nativeObjectEventCount();
    private static native double nativeGm82CompatCheck();
    private static native double nativeGm82ColorReverse(double color);
    private static native double nativeGm82ColorInverse(double color);
    private static native int nativeGm82TokenStart(String text, String separator);
    private static native String nativeGm82TokenNext();
    private static native void nativeGm82TokenReset();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        nativeRuntimeCreate(640, 480);

        try {
            java.io.File webViewDir = new java.io.File(getCacheDir(), "WebView");
            if (!webViewDir.exists()) webViewDir.mkdirs();
            java.io.File defaultDir = new java.io.File(webViewDir, "Default");
            if (!defaultDir.exists()) defaultDir.mkdirs();
            java.io.File httpCacheDir = new java.io.File(defaultDir, "HTTP Cache");
            if (!httpCacheDir.exists()) httpCacheDir.mkdirs();
            java.io.File codeCacheDir = new java.io.File(httpCacheDir, "Code Cache");
            if (!codeCacheDir.exists()) codeCacheDir.mkdirs();
            java.io.File wasmDir = new java.io.File(codeCacheDir, "wasm");
            if (!wasmDir.exists()) wasmDir.mkdirs();
        } catch (Exception ignored) {}

        AudioAttributes audioAttributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        soundPool = new SoundPool.Builder().setMaxStreams(32).setAudioAttributes(audioAttributes).build();

        FrameLayout rootLayout = new FrameLayout(this);
        rootLayout.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        webView = new WebView(this);
        WebSettings settings = webView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setDatabaseEnabled(true);
        settings.setAllowFileAccess(true);
        settings.setAllowContentAccess(true);
        settings.setAllowFileAccessFromFileURLs(true);
        settings.setAllowUniversalAccessFromFileURLs(true);
        settings.setMediaPlaybackRequiresUserGesture(false);
        settings.setCacheMode(WebSettings.LOAD_DEFAULT);
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
        settings.setUseWideViewPort(true);
        settings.setLoadWithOverviewMode(false);
        settings.setSupportZoom(true);
        settings.setBuiltInZoomControls(true);
        settings.setDisplayZoomControls(false);
        webView.setScrollBarStyle(WebView.SCROLLBARS_INSIDE_OVERLAY);
        webView.setFocusable(true);
        webView.setFocusableInTouchMode(true);
        webView.requestFocus();

        final LinearLayout splash = new LinearLayout(this);
        splash.setOrientation(LinearLayout.VERTICAL);
        splash.setGravity(Gravity.CENTER);
        splash.setBackgroundColor(Color.rgb(24, 35, 48)); // #182330
        ImageView splashIcon = new ImageView(this);
        splashIcon.setImageResource(R.mipmap.ic_launcher);
        splashIcon.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        int iconSize = (int)(72 * getResources().getDisplayMetrics().density);
        splash.addView(splashIcon, new LinearLayout.LayoutParams(iconSize, iconSize));
        TextView splashTitle = new TextView(this);
        splashTitle.setText("NOR Maker 0.1");
        splashTitle.setTextColor(Color.WHITE);
        splashTitle.setTextSize(20);
        splashTitle.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams titleLp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        titleLp.topMargin = (int)(16 * getResources().getDisplayMetrics().density);
        splash.addView(splashTitle, titleLp);

        webView.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                return false;
            }

            @Override
            public void onPageFinished(WebView view, String url) {
                super.onPageFinished(view, url);
                splash.animate().alpha(0f).setDuration(300).withEndAction(new Runnable() {
                    @Override
                    public void run() {
                        splash.setVisibility(View.GONE);
                    }
                });
            }

            @Override
            public boolean onRenderProcessGone(WebView view, RenderProcessGoneDetail detail) {
                android.util.Log.e("NOR_WEBVIEW", "Render process gone; reloading webView");
                if (view != null) {
                    view.post(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                view.loadUrl("file:///android_asset/www/index.html");
                            } catch (Exception ignored) {}
                        }
                    });
                }
                return true;
            }
        });
        webView.setWebChromeClient(new WebChromeClient() {
            @Override
            public boolean onConsoleMessage(android.webkit.ConsoleMessage consoleMessage) {
                android.util.Log.d("NOR_WEBVIEW", consoleMessage.message() + " -- Line " + consoleMessage.lineNumber());
                return true;
            }

            @Override
            public boolean onShowFileChooser(WebView view, ValueCallback<Uri[]> callback, FileChooserParams params) {
                if (pendingFileCallback != null) pendingFileCallback.onReceiveValue(null);
                pendingFileCallback = callback;
                try {
                    Intent intent = params.createIntent();
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, params.getMode() == FileChooserParams.MODE_OPEN_MULTIPLE);
                    startActivityForResult(intent, REQUEST_FILE_CHOOSER);
                    return true;
                } catch (Exception error) {
                    pendingFileCallback = null;
                    callback.onReceiveValue(null);
                    return false;
                }
            }
        });
        webView.addJavascriptInterface(new NativeBridge(), "NorNative");
        webView.loadUrl("file:///android_asset/www/index.html");

        rootLayout.addView(webView, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        rootLayout.addView(splash, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        setContentView(rootLayout);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_FILE_CHOOSER) {
            if (pendingFileCallback == null) return;
            Uri[] results = null;
            if (resultCode == RESULT_OK && data != null) {
                if (data.getClipData() != null) {
                    int count = data.getClipData().getItemCount();
                    results = new Uri[count];
                    for (int i = 0; i < count; i++) results[i] = data.getClipData().getItemAt(i).getUri();
                } else if (data.getData() != null) {
                    results = new Uri[]{data.getData()};
                }
            }
            pendingFileCallback.onReceiveValue(results);
            pendingFileCallback = null;
            return;
        }
        if (requestCode != REQUEST_GMX_GMZ || resultCode != RESULT_OK || data == null || data.getData() == null) return;
        Uri uri = data.getData();
        try {
            File inbox = new File(getFilesDir(), "inbox");
            if (!inbox.exists() && !inbox.mkdirs()) return;
            String name = uri.getLastPathSegment();
            if (name == null || name.length() == 0) name = "project.gmz";
            name = name.replaceAll("[^A-Za-z0-9._-]", "_");
            File target = new File(inbox, name);
            try (InputStream in = getContentResolver().openInputStream(uri);
                 FileOutputStream out = new FileOutputStream(target)) {
                if (in == null) return;
                byte[] buffer = new byte[8192];
                int n;
                while ((n = in.read(buffer)) >= 0) { if (n > 0) out.write(buffer, 0, n); }
            }
            final String path = target.getAbsolutePath().replace("\\", "\\\\").replace("'", "\\'");
            if (webView != null) webView.post(() -> webView.evaluateJavascript("window.__norGmxGmzPicked && window.__norGmxGmzPicked('" + path + "')", null));
        } catch (Exception ignored) { }
    }

    @Override
    protected void onDestroy() {
        releaseSoundResources();
        nativeRuntimeDestroy();
        super.onDestroy();
    }

    /** Convert Android KeyEvent constants to the virtual-key values used by GM8 GML. */
    private static int toGm82KeyCode(int androidCode) {
        switch (androidCode) {
            case KeyEvent.KEYCODE_DPAD_LEFT: return 37;
            case KeyEvent.KEYCODE_DPAD_UP: return 38;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return 39;
            case KeyEvent.KEYCODE_DPAD_DOWN: return 40;
            case KeyEvent.KEYCODE_ENTER: return 13;
            case KeyEvent.KEYCODE_SPACE: return 32;
            case KeyEvent.KEYCODE_ESCAPE: return 27;
            case KeyEvent.KEYCODE_BACK: return 27;
            case KeyEvent.KEYCODE_DEL: return 8;
            case KeyEvent.KEYCODE_TAB: return 9;
            case KeyEvent.KEYCODE_SHIFT_LEFT:
            case KeyEvent.KEYCODE_SHIFT_RIGHT: return 16;
            case KeyEvent.KEYCODE_CTRL_LEFT:
            case KeyEvent.KEYCODE_CTRL_RIGHT: return 17;
            case KeyEvent.KEYCODE_ALT_LEFT:
            case KeyEvent.KEYCODE_ALT_RIGHT: return 18;
            default:
                // Android keycodes for A-Z and 0-9 are contiguous, while GM8 uses ASCII.
                if (androidCode >= KeyEvent.KEYCODE_A && androidCode <= KeyEvent.KEYCODE_Z) {
                    return 'A' + (androidCode - KeyEvent.KEYCODE_A);
                }
                if (androidCode >= KeyEvent.KEYCODE_0 && androidCode <= KeyEvent.KEYCODE_9) {
                    return '0' + (androidCode - KeyEvent.KEYCODE_0);
                }
                return androidCode;
        }
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        int action = event.getAction();
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK && action == KeyEvent.ACTION_UP) {
            if (webView != null) {
                webView.evaluateJavascript("if (window.__handleBackPress) { window.__handleBackPress(); } else { window.history.back(); }", null);
                return true;
            }
        }
        if (action == KeyEvent.ACTION_DOWN || action == KeyEvent.ACTION_UP) {
            nativeRuntimeKey(toGm82KeyCode(event.getKeyCode()), action == KeyEvent.ACTION_DOWN);
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public void onBackPressed() {
        if (webView != null && webView.canGoBack()) {
            webView.goBack();
        } else {
            // Keep app in foreground or minimize without crashing
            moveTaskToBack(true);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (webView != null) {
            webView.onResume();
            webView.resumeTimers();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (webView != null) {
            webView.onPause();
            webView.pauseTimers();
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (!hasFocus) {
            // Prevent a held key from remaining active after dialogs, file pickers, or app switching.
            for (int code = 0; code < 256; code++) nativeRuntimeKey(code, false);
        }
    }

    private void drainNativeSoundCommands() {
        if (soundPool == null) return;
        try {
            JSONArray commands = new JSONArray(nativeRuntimeConsumeSoundCommands());
            for (int i = 0; i < commands.length(); i++) {
                JSONArray command = commands.optJSONArray(i);
                if (command == null || command.length() < 5) continue;
                int kind = command.optInt(0, 0);
                int soundId = command.optInt(1, -1);
                int loop = command.optInt(2, 0);
                float volume = (float) command.optDouble(4, 1.0);
                Integer resource = soundResources.get(soundId);
                if (kind == 1 && resource != null) {
                    int stream = soundPool.play(resource, volume, volume, 1, loop != 0 ? -1 : 0, 1.0f);
                    if (stream != 0) soundStreams.put(soundId, stream);
                } else if (kind == 2) {
                    Integer stream = soundStreams.remove(soundId);
                    if (stream != null) soundPool.stop(stream);
                } else if (kind == 3) {
                    Integer stream = soundStreams.get(soundId);
                    if (stream != null) soundPool.setVolume(stream, volume, volume);
                }
            }
        } catch (Exception ignored) { }
    }

    private boolean registerSoundResource(int soundId, String path) {
        if (soundPool == null || soundId < 0 || path == null || path.length() == 0) return false;
        try {
            Integer old = soundResources.remove(soundId);
            if (old != null) soundPool.unload(old);
            int resource = soundPool.load(path, 1);
            if (resource == 0) return false;
            soundResources.put(soundId, resource);
            return true;
        } catch (Exception ignored) { return false; }
    }

    private void clearSoundResources() {
        if (soundPool == null) return;
        for (Integer stream : soundStreams.values()) soundPool.stop(stream);
        soundStreams.clear();
        for (Integer resource : soundResources.values()) soundPool.unload(resource);
        soundResources.clear();
    }

    private void releaseSoundResources() {
        if (soundPool == null) return;
        clearSoundResources();
        soundPool.release();
        soundPool = null;
    }

    private static String mimeForName(String name) {
        String n = name.toLowerCase();
        if (n.endsWith(".png")) return "image/png";
        if (n.endsWith(".jpg") || n.endsWith(".jpeg")) return "image/jpeg";
        if (n.endsWith(".wav")) return "audio/wav";
        if (n.endsWith(".mp3")) return "audio/mpeg";
        if (n.endsWith(".ogg")) return "audio/ogg";
        if (n.endsWith(".gmx") || n.endsWith(".gml")) return "text/plain";
        return "application/octet-stream";
    }

    private static void collectGmxFiles(File root, File current, JSONArray out) throws Exception {
        File[] children = current.listFiles();
        if (children == null) return;
        for (File file : children) {
            if (file.isDirectory()) { collectGmxFiles(root, file, out); continue; }
            if (!file.isFile() || file.length() > 32L * 1024L * 1024L) continue;
            try (InputStream in = new java.io.FileInputStream(file); ByteArrayOutputStream bytes = new ByteArrayOutputStream()) {
                byte[] buffer = new byte[8192]; int n;
                while ((n = in.read(buffer)) >= 0) if (n > 0) bytes.write(buffer, 0, n);
                String relative = root.toURI().relativize(file.toURI()).getPath();
                JSONObject item = new JSONObject();
                item.put("name", file.getName());
                item.put("webkitRelativePath", relative);
                item.put("type", mimeForName(file.getName()));
                item.put("dataUrl", "data:" + mimeForName(file.getName()) + ";base64," + Base64.encodeToString(bytes.toByteArray(), Base64.NO_WRAP));
                out.put(item);
            }
        }
    }

    public final class NativeBridge {
        @JavascriptInterface public void step(float delta) { nativeRuntimeStep(delta); drainNativeSoundCommands(); }
        @JavascriptInterface public boolean registerSound(int soundId, String path) { return registerSoundResource(soundId, path); }
        @JavascriptInterface public void clearSounds() { clearSoundResources(); }
        @JavascriptInterface public String snapshot() { return nativeRuntimeSnapshotJson(); }
        @JavascriptInterface public String evaluateGml(String source) { return nativeEvaluateGml(source); }
        @JavascriptInterface public boolean registerEvent(int objectId, int mainType, int subType, String source) {
            return nativeRuntimeRegisterEvent(objectId, mainType, subType, source);
        }
        @JavascriptInterface public boolean registerObject(int objectId, String name) {
            return nativeRuntimeRegisterObject(objectId, name);
        }
        @JavascriptInterface public boolean registerScript(String name, String source) {
            return nativeRuntimeRegisterScript(name, source);
        }
        @JavascriptInterface public void clearScripts() { nativeRuntimeClearScripts(); }
        @JavascriptInterface public void clearEvents() { nativeRuntimeClearEvents(); }
        @JavascriptInterface public int addInstance(int objectId, int spriteId, int spriteWidth, int spriteHeight, int spriteSubimages, float x, float y, float vx, float vy) {
            return nativeRuntimeAddInstance(objectId, spriteId, spriteWidth, spriteHeight, spriteSubimages, x, y, vx, vy);
        }
        @JavascriptInterface public boolean setSpriteBitmapBase64(int spriteId, int frame, int width, int height, String rgbaBase64) {
            if (rgbaBase64 == null || rgbaBase64.length() == 0) return false;
            try {
                byte[] rgba = Base64.decode(rgbaBase64, Base64.DEFAULT);
                return nativeRuntimeSetSpriteBitmap(spriteId, frame, width, height, rgba);
            } catch (IllegalArgumentException error) { return false; }
        }
        public boolean renderNativeBitmap(Bitmap target) { return target != null && nativeRuntimeRenderBitmap(target); }
        @JavascriptInterface public void clearInstances() { nativeRuntimeClearInstances(); }
        @JavascriptInterface public void clearRoomTransient() { nativeRuntimeClearRoomTransient(); }
        @JavascriptInterface public boolean executeGml(int instanceId, String source) {
            return nativeRuntimeExecuteGml(instanceId, source);
        }
        @JavascriptInterface public String consumeSoundCommands() { return nativeRuntimeConsumeSoundCommands(); }
        @JavascriptInterface public int room() { return nativeRuntimeGetRoom(); }
        @JavascriptInterface public void clearResourceRegistry() { nativeClearResourceRegistry(); }
        @JavascriptInterface public int registerResource(int kind, int id, String name, int width, int height, int frames) {
            return nativeRegisterResource(kind, id, name, width, height, frames);
        }
        @JavascriptInterface public int resourceCount() { return nativeResourceCount(); }
        @JavascriptInterface public int registerObjectEvent(int objectId, int mainType, int subType, String source) {
            return nativeRegisterObjectEvent(objectId, mainType, subType, source);
        }
        @JavascriptInterface public int objectEventCount() { return nativeObjectEventCount(); }
        @JavascriptInterface public double gm82CompatCheck() { return nativeGm82CompatCheck(); }
        @JavascriptInterface public double gm82ColorReverse(double color) { return nativeGm82ColorReverse(color); }
        @JavascriptInterface public double gm82ColorInverse(double color) { return nativeGm82ColorInverse(color); }
        @JavascriptInterface public int gm82TokenStart(String text, String separator) { return nativeGm82TokenStart(text, separator); }
        @JavascriptInterface public String gm82TokenNext() { return nativeGm82TokenNext(); }
        @JavascriptInterface public void gm82TokenReset() { nativeGm82TokenReset(); }
        @JavascriptInterface public String coreIdentity() { return nativeCoreIdentity(); }
        @JavascriptInterface public boolean validateGmk(byte[] bytes) { return nativeValidateGmk(bytes); }
        @JavascriptInterface public String gmkHeaderJson(byte[] bytes) { return nativeGmkHeaderJson(bytes); }
        @JavascriptInterface public String gmkLayoutJson(byte[] bytes) { return nativeGmkLayoutJson(bytes); }
        @JavascriptInterface public String gmkChunkInventory(byte[] bytes) { return nativeGmkChunkInventory(bytes); }
        @JavascriptInterface public int compileGml(String source) { return nativeCompileGml(source); }
        @JavascriptInterface public boolean codeExists(int codeId) { return nativeCodeExists(codeId); }
        @JavascriptInterface public int codeArgCount(int codeId) { return nativeCodeGetArgCount(codeId); }
        @JavascriptInterface public void destroyCode(int codeId) { nativeCodeDestroy(codeId); }
        @JavascriptInterface public int executeCode(int instanceId, int codeId) { return nativeCodeExecute(instanceId, codeId); }
        @JavascriptInterface public boolean exportRom(String title, String outputPath, int kind) { return nativeExportRom(title, outputPath, kind); }
        @JavascriptInterface public int detectRom(String path) { return nativeDetectRom(path); }
        @JavascriptInterface public boolean validateRom(String path, int kind) { return nativeValidateRom(path, kind); }
        @JavascriptInterface public String runRuntimeSmokeTest() {
            boolean created = nativeRuntimeCreate(320, 240);
            int id = created ? nativeRuntimeAddInstance(1, -1, 1, 1, 1, 10f, 20f, 30f, 0f) : -1;
            String before = nativeRuntimeSnapshotJson();
            nativeRuntimeStep(0.1f);
            String moved = nativeRuntimeSnapshotJson();
            boolean movedForward = moved != null && !moved.equals(before);
            nativeRuntimeSetRoom(2, 160, 120, true);
            String room = nativeRuntimeSnapshotJson();
            boolean roomChanged = room != null && room.contains("\"room\":2");
            nativeRuntimeDestroy();
            boolean recreated = nativeRuntimeCreate(64, 64);
            String fresh = nativeRuntimeSnapshotJson();
            boolean clean = fresh != null && !fresh.contains("\"instances\":[{");
            nativeRuntimeDestroy();
            return "{\"created\":" + created + ",\"instanceId\":" + id + ",\"moved\":" + movedForward + ",\"roomChanged\":" + roomChanged + ",\"recreated\":" + recreated + ",\"clean\":" + clean + "}";
        }
        @JavascriptInterface public boolean exportNorJson(String json, String outputPath) {
            return nativeExportNorJson(json, outputPath);
        }
        @JavascriptInterface public String exportNorJsonToApp(String json, String fileName) {
            if (json == null || fileName == null || fileName.length() == 0) return "";
            String safeName = fileName.replaceAll("[^A-Za-z0-9._-]", "_");
            File target = new File(getFilesDir(), safeName);
            return nativeExportNorJson(json, target.getAbsolutePath()) ? target.getAbsolutePath() : "";
        }
        @JavascriptInterface public void setRoom(int id, int width, int height, boolean clear) {
            nativeRuntimeSetRoom(id, width, height, clear);
        }
        @JavascriptInterface public String importGmxGmzToApp(String archivePath, String folderName) {
            if (archivePath == null || archivePath.length() == 0) return "";
            String safeName = (folderName == null || folderName.length() == 0 ? "gmx_import" : folderName)
                    .replaceAll("[^A-Za-z0-9._-]", "_");
            File target = new File(new File(getFilesDir(), "imports"), safeName);
            if (!target.exists() && !target.mkdirs()) return "";
            return nativeImportGmxGmz(archivePath, target.getAbsolutePath()) != 0
                    ? target.getAbsolutePath() : "";
        }
        @JavascriptInterface public String readGmxGmzFiles(String extractedDir) {
            try {
                File root = new File(extractedDir);
                if (!root.isDirectory()) return "";
                JSONObject payload = new JSONObject();
                JSONArray files = new JSONArray();
                collectGmxFiles(root, root, files);
                payload.put("root", root.getAbsolutePath());
                payload.put("files", files);
                return payload.toString();
            } catch (Exception ignored) { return ""; }
        }
        @JavascriptInterface public void pickGmxGmz() {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("application/octet-stream");
            intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"application/octet-stream", "application/zip", "text/xml"});
            startActivityForResult(intent, REQUEST_GMX_GMZ);
        }
        @JavascriptInterface public boolean exportGmxGmz(String sourceDir, String outputPath, String kind) {
            if (sourceDir == null || outputPath == null) return false;
            String mode = "gmz".equalsIgnoreCase(kind) ? "gmz" : "gmx";
            return nativeExportGmxGmz(sourceDir, outputPath, mode) != 0;
        }
    }
}
