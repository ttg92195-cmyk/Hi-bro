# Add project specific ProGuard rules here.

# Keep native methods
-keepclassmembers class * {
    native <methods>;
}

# Keep game activity
-keep class com.eosshooter.GameActivity { *; }
