CREATE TABLE exchange_rate
(
    id SERIAL PRIMARY KEY,

    curr TEXT NOT NULL,

    rate DOUBLE PRECISION NOT NULL,

    date DATE NOT NULL,

    base TEXT NOT NULL
);
