import React, { useState, useEffect } from 'react';
import { LineChart, Line, XAxis, YAxis, Tooltip, Legend, ResponsiveContainer, ReferenceDot, CartesianGrid } from 'recharts';
import { useNavigate } from 'react-router-dom';
import dummyData from '../data/dummyData.json';  // Import JSON

function PlantDetailsScreen() {
    const [trendData, setTrendData] = useState([]);
    const navigate = useNavigate();

    useEffect(() => {
        setTrendData(dummyData.plant.trendData);  // Load trend data from JSON
    }, []);

    return (
        <div className="details-screen">
            <button className="back-button" onClick={() => navigate(-1)}>Back</button>
            <h2>Plant Trend Analysis</h2>

            <ResponsiveContainer width="100%" height={350}>
                <LineChart data={trendData} margin={{ top: 20, right: 30, left: 20, bottom: 10 }}>
                    <CartesianGrid strokeDasharray="3 3" />
                    <XAxis dataKey="time" />
                    <YAxis />
                    <Tooltip />
                    <Legend />

                    {/* Plot Temperature, Humidity, Light, and Moisture */}
                    <Line type="monotone" dataKey="temperature" stroke="#ff7300" strokeWidth={2} name="Temperature (°C)" />
                    <Line type="monotone" dataKey="humidity" stroke="#387908" strokeWidth={2} name="Humidity (%)" />
                    <Line type="monotone" dataKey="light" stroke="#8884d8" strokeWidth={2} name="Light (%)" />
                    <Line type="monotone" dataKey="moisture" stroke="#3498db" strokeWidth={2} name="Moisture (%)" />

                    {/* Mark Water Level Change Events */}
                    {trendData.map((entry, index) => {
                        if (index > 0 && entry.waterLevel > trendData[index - 1].waterLevel) {
                            return (
                                <ReferenceDot
                                    key={index}
                                    x={entry.time}
                                    y={entry.moisture}
                                    r={6}
                                    fill="blue"
                                    stroke="white"
                                    label={{ value: "Watered", position: "top", dy: -10, fill: "blue", fontSize: 12 }}
                                />
                            );
                        }
                        return null;
                    })}
                </LineChart>
            </ResponsiveContainer>
        </div>
    );
}

export default PlantDetailsScreen;
