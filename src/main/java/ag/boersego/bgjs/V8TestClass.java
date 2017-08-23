package ag.boersego.bgjs;

import android.util.Log;

/**
 * Created by martin on 13.04.17.
 */

public class V8TestClass extends V8Object {
    public V8TestClass(V8Engine engine) {
        super(engine);
        Log.d("V8TestClass", "Constructor");
    }

    public V8TestClass(V8Engine engine, long jsObjPtr) {
        super(engine, jsObjPtr);
        Log.d("V8TestClass", "Constructor:");
    }

    public native void test(long testL, float testF, double testD, String str);
    public native String getName();
    public void test3(long testL, float testF) {
        Log.d("V8TestClass", "Hello:" + Long.toString(testL) + ":" + Float.toString(testF));
    }
    public long testLong;

    public static long staticField;
    public static void test4(V8TestClass cls) {
        Log.d("V8TestClass", "Static:" + cls.getName());
    }
}
