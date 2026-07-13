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
    fees_accrued DOUBLE PRECISION DEFAULT 0.0,
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
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    strategy_id INTEGER REFERENCES strategy(id) ON DELETE CASCADE
);

DROP TABLE IF EXISTS ledger_entry CASCADE;
DROP TYPE IF EXISTS ledger_entry_type CASCADE;

-- 1. Create the custom ENUM type exactly matching your C enum values
CREATE TYPE ledger_entry_type AS ENUM (
    'ASSET',
    'EXPENSE',
    'LIABILITY',
    'EQUITY',
    'REVENUE'
);

CREATE TABLE ledger_entry (
    id SERIAL PRIMARY KEY,
    type ledger_entry_type NOT NULL,
    account_name VARCHAR(100) NOT NULL,
    debit DOUBLE PRECISION DEFAULT 0.0,
    credit DOUBLE PRECISION DEFAULT 0.0,
    memo VARCHAR(100),
    currency VARCHAR(10) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    strategy_id INTEGER REFERENCES strategy(id) ON DELETE CASCADE
);


DROP TABLE IF EXISTS bank_account CASCADE;
CREATE TABLE bank_account (
    id SERIAL PRIMARY KEY,
    symbol VARCHAR(100) NULL,
    balance DOUBLE PRECISION DEFAULT 0.0,
    currency VARCHAR(10) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    strategy_id INTEGER REFERENCES strategy(id) ON DELETE CASCADE
);


DROP TABLE IF EXISTS fno_position CASCADE;
DROP TYPE IF EXISTS instrument_type CASCADE;
DROP TYPE IF EXISTS opt_type CASCADE;

CREATE TYPE opt_type AS ENUM (
    'PE',
    'CE',
    'NA'
);

CREATE TYPE instrument_type AS ENUM (
    'OPTIDX',
    'OPTSTK',
    'FUTIDX',
    'FUTSTK'
);

CREATE TABLE fno_position (
    id SERIAL PRIMARY KEY,                                      
    sys_id VARCHAR(100), -- maps to char id[100]
    symbol VARCHAR(100) NOT NULL,
    qty INTEGER NOT NULL,
    price DOUBLE PRECISION NOT NULL,
    ltp DOUBLE PRECISION NOT NULL,
    expiry DATE NOT NULL,                                              
    strike DOUBLE PRECISION NOT NULL,
    opt_type opt_type NOT NULL,
    inst_type instrument_type NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    strategy_id INTEGER REFERENCES strategy(id) ON DELETE CASCADE
);


DROP TABLE IF EXISTS fno_trade CASCADE;
DROP TYPE IF EXISTS trans_type CASCADE;

-- 1. Create the Transaction Type ENUM
CREATE TYPE trans_type AS ENUM (
    'MB',   -- Market Buy
    'MS',   -- Market Sell
    'LB',   -- Limit Buy
    'LS',   -- Limit Sell
    'MOB',  -- Market Order Buy
    'MCB',  -- Market Cover Buy
    'MOS',  -- Market Order Sell
    'MCS',  -- Market Cover Sell
    'FSO',  -- F&O Sell Open
    'FSC',  -- F&O Sell Close
    'FBO',  -- F&O Buy Open
    'FBC'   -- F&O Buy Close
);

-- 2. Create the FNO Trade table
CREATE TABLE fno_trade (
    id SERIAL PRIMARY KEY,                                             
    symbol VARCHAR(100) NOT NULL,
    broker_code VARCHAR(100) NOT NULL,
    trade_date DATE NOT NULL,                                          
    strategy_symbol VARCHAR(100) NOT NULL,
    expiry DATE NOT NULL,                                              
    opt_type opt_type NOT NULL,                                   
    inst_type instrument_type NOT NULL,                           
    qty INTEGER NOT NULL,
    price DOUBLE PRECISION NOT NULL,                                   
    brokerage DOUBLE PRECISION DEFAULT 0.0,
    service_tax DOUBLE PRECISION DEFAULT 0.0,
    strike DOUBLE PRECISION NOT NULL,
    trans_type trans_type NOT NULL,                               
    currency VARCHAR(10) NOT NULL,                                     
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    strategy_id INTEGER REFERENCES strategy(id) ON DELETE CASCADE
);


DROP TABLE IF EXISTS fno_bhav CASCADE;
CREATE TABLE fno_bhav (
    id SERIAL PRIMARY KEY,
    symbol VARCHAR(100) NOT NULL,
    ltp DOUBLE PRECISION NOT NULL,
    expiry DATE NOT NULL,
    strike DOUBLE PRECISION NOT NULL,
    opt_type opt_type NOT NULL,
    inst_type instrument_type NOT NULL,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    strategy_id INTEGER REFERENCES strategy(id) ON DELETE CASCADE
);


DROP TABLE IF EXISTS equity_trade CASCADE;

CREATE TABLE equity_trade (
    id SERIAL PRIMARY KEY,                                             
    symbol VARCHAR(100) NOT NULL,                                      
    broker_code VARCHAR(100) NOT NULL,
    trade_date DATE NOT NULL,                                          
    strategy_symbol VARCHAR(100) NOT NULL,
    qty INTEGER NOT NULL,
    price DOUBLE PRECISION NOT NULL,                                   
    brokerage DOUBLE PRECISION DEFAULT 0.0,
    service_tax DOUBLE PRECISION DEFAULT 0.0,
    trans_type trans_type_enum NOT NULL,                               
    currency VARCHAR(10) NOT NULL,                                    
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,     
    strategy_id INTEGER REFERENCES strategy(id) ON DELETE CASCADE,     
);

-- Index for scanning trades filtered by symbol (ISIN) and strategy quickly
CREATE INDEX idx_equity_trade_lookup ON equity_trade (strategy_id, symbol);


DROP TABLE IF EXISTS position_equity CASCADE;

CREATE TABLE position_equity (
    id SERIAL PRIMARY KEY,                                             
    sys_id VARCHAR(100),
    isin VARCHAR(100) NOT NULL,
    symbol VARCHAR(100) NOT NULL,
    qty INTEGER NOT NULL,
    price DOUBLE PRECISION NOT NULL,                                   
    ltp DOUBLE PRECISION NOT NULL,                                     
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,      
    strategy_id INTEGER REFERENCES strategy(id) ON DELETE CASCADE     
);

-- Index for blistering fast lookups when validating a specific stock position inside a strategy loop
CREATE INDEX idx_position_equity_lookup ON position_equity (strategy_id, symbol);

DROP TABLE IF EXISTS bhav CASCADE;

CREATE TABLE equity_bhav (
    symbol VARCHAR(100) PRIMARY KEY,
    ltp DOUBLE PRECISION NOT NULL,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

ALTER TABLE exchange_rate 
ADD CONSTRAINT unique_exchange_rate UNIQUE (curr, base, date);
