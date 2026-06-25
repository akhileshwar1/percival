-- Ensure clean state if re-running migrations
DROP TABLE IF EXISTS exchange_rate CASCADE;
CREATE TABLE exchange_rate
(
    id SERIAL PRIMARY KEY,
    curr TEXT NOT NULL,
    rate DOUBLE PRECISION NOT NULL,
    date DATE NOT NULL,
    base TEXT NOT NULL
);

DROP TABLE IF EXISTS strategy CASCADE;
CREATE TABLE strategy
(
    id SERIAL PRIMARY KEY,    
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    symbol VARCHAR(100) NOT NULL,
    cash DOUBLE PRECISION DEFAULT 0.0,
    nav DOUBLE PRECISION DEFAULT 100.0,
    curr_pos_index INTEGER DEFAULT -1,
    curr_f_pos_index INTEGER DEFAULT -1,
    curr_investor_index INTEGER DEFAULT -1,
    curr_entry_id INTEGER DEFAULT -1,
    curr_journal_id INTEGER DEFAULT -1 
);

DROP TABLE IF EXISTS investor CASCADE;
CREATE TABLE investor (
    id SERIAL PRIMARY KEY,
    symbol VARCHAR(100) NULL,
    cash DOUBLE PRECISION NULL,
    name VARCHAR(100) NOT NULL,
    units DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    inception_date DATE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
