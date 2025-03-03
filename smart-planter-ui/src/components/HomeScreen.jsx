import React, { useState, useEffect } from 'react';
import dummyData from '../data/dummyData.json';
import { motion } from "framer-motion";
import { Link } from 'react-router-dom';

function HomeScreen() {
    const [plant, setPlant] = useState({
        name: "",
        temperature: 0,
        humidity: 0,
        waterLevel: 100, // Default high value
        light: 0,
        moisture: 100, // Default high value
        image: "",
        trendData: []
    });

    useEffect(() => {
        setPlant(dummyData.plant);
        console.log("Plant data loaded:", dummyData.plant); // Debugging
    }, []);

    return (
        <motion.div
            className="home-screen"
            initial={{ opacity: 0, y: 10 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: -10 }}
            transition={{ duration: 0.5 }}
        >
            <h2>{plant.name}</h2>
            <img className="plant-image" src={`/${plant.image}`} alt={plant.name} />
            <div className="data">
                <div className="data-item"><span>Temperature:</span> {plant.temperature}°C</div>
                <div className="data-item"><span>Humidity:</span> {plant.humidity}%</div>
                <div className="data-item"><span>Water Level:</span> {plant.waterLevel}%</div>
                <div className="data-item"><span>Light:</span> {plant.light}%</div>
                <div className="data-item"><span>Moisture:</span> {plant.moisture}%</div>
            </div>
            <div className="alerts">
                {plant?.waterLevel < 25 && <p>⚠ Refill the water tank!</p>}
                {plant?.moisture < 20 && <p>⚠ Moisture levels are too low!</p>}
            </div>
            <nav>
                <Link to="/details" className="button">Plant Details</Link>
                <Link to="/settings" className="button">Settings</Link>
            </nav>
        </motion.div>
    );
}

export default HomeScreen;
