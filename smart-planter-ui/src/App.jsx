import React from 'react';
import { BrowserRouter as Router, Routes, Route, useLocation } from 'react-router-dom';
import { AnimatePresence } from "framer-motion";
import HomeScreen from './components/HomeScreen';
import DetailsScreen from './components/DetailsScreen';
import SettingsScreen from './components/SettingsScreen';
import './styles.css'; // Ensure styles.css is in src/

// Main App Component
function App() {
    return (
        <Router>
            <AnimatedRoutes />
        </Router>
    );
}

// Animated Route Transitions
function AnimatedRoutes() {
    const location = useLocation(); // Track route changes

    return (
        <AnimatePresence mode="wait">
            <Routes location={location} key={location.pathname}>
                <Route path="/" element={<HomeScreen />} />
                <Route path="/details" element={<DetailsScreen />} />
                <Route path="/settings" element={<SettingsScreen />} />
            </Routes>
        </AnimatePresence>
    );
}

export default App;
