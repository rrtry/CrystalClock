//import com.android.build.api.variant.FilterConfiguration

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

//val abiCodes = mapOf("armeabi-v7a" to 1, "x86" to 2, "x86_64" to 3, "arm64-v8a" to 4);

android {
    namespace = "com.rrtry.crystalclock"
    compileSdk = 35

    signingConfigs {
        create("release-cfg") {
            keyAlias      = "key0"
            keyPassword   = System.getenv("ANDROID_KEYSTORE_KEY_PASSWORD")
            storeFile     = file(System.getenv("ANDROID_KEYSTORE_PATH"))
            storePassword = System.getenv("ANDROID_KEYSTORE_PASSWORD")

            enableV1Signing = true
            enableV2Signing = true
        }
    }

    defaultConfig {
        applicationId = "com.rrtry.crystalclock"
        minSdk = 26
        targetSdk = 35
        versionCode = 104
        versionName = "1.0.4"

        externalNativeBuild {
            cmake {
                val glVersion = "ES20"
                arguments += "-DGL_VERSION=$glVersion"
                arguments += "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON"
                cppFlags += "-std=c++11"
            }
        }
        signingConfig = signingConfigs.getByName("release-cfg")
    }

    /*
    splits {
        abi {
            this.isEnable = true
            reset()
            this.include("x86", "x86_64", "arm64-v8a", "armeabi-v7a")
            this.isUniversalApk = false
        }
    }
     */

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("release-cfg")
            ndk {
                abiFilters.clear()
                abiFilters.addAll(listOf("x86", "x86_64", "arm64-v8a", "armeabi-v7a"))
            }
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}
/*
androidComponents {
    onVariants { variant ->
        val baseVersionCode = variant.outputs.first().versionCode.get()?.toInt() ?: 1
        variant.outputs.forEach { output ->
            val abiFilter = output.filters.find { it.filterType == FilterConfiguration.FilterType.ABI }
            val baseAbiVersionCode = abiFilter?.identifier?.let { abiCodes[it] }
            if (baseAbiVersionCode != null) {
                output.versionCode.set(baseAbiVersionCode * 1000 + baseVersionCode)
            }
        }
    }
}
 */