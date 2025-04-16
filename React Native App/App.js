import React, { useEffect } from 'react';
import { View, Text, Image, StyleSheet } from 'react-native';
import { WebView } from 'react-native-webview';
import * as Font from 'expo-font';
import * as SplashScreen from 'expo-splash-screen';

const customFonts = {
    'Poppins-Bold': require('./assets/fonts/Poppins-Bold.ttf'),
};

export default function App() {
    const [fontsLoaded] = Font.useFonts(customFonts);

    useEffect(() => {
        async function prepare() {
            if (fontsLoaded) {
                await SplashScreen.hideAsync();
            }
        }
        prepare();
    }, [fontsLoaded]);

    if (!fontsLoaded) {
        return null; // wait until fonts are loaded
    }

    return (
        <View style={{ flex: 1 }}>
            {/* Header */}
            <View style={styles.header}>
                <View style={styles.row}>
                    <Image
                        source={require('./assets/logo.png')}
                        style={styles.logo}
                        resizeMode="contain"
                    />
                    <Text style={styles.title}>Smart Planter</Text>
                </View>
            </View>

            {/* WebView */}
            <WebView
                source={{ uri: 'http://192.168.8.172:1880/dashboard' }}
                style={{ flex: 1 }}
            />
        </View>
    );
}

const styles = StyleSheet.create({
    header: {
        paddingTop: 50,
        paddingBottom: 20,
        backgroundColor: '#f0f5f1',
        alignItems: 'center',
    },
    row: {
        flexDirection: 'row',
        alignItems: 'center',
        justifyContent: 'center',
    },
    logo: {
        height: 40,
        width: 40,
        marginRight: 10,
    },
    title: {
        fontSize: 22,
        fontFamily: 'Poppins-Bold',
        color: '#2b7a0b',
    },
});
