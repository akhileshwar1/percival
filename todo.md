## TODO:
* State not on stack.
* Same journalid for cashflow and some other entry.
* Clean logs.
* Dividend route.
* String instead of char *.
* offboarding.
* Don't reverse state and reapply instead apply again on the current state.
* Parse comma seperate nos like 53,327 from csv.
* saturday sunday fee accrual.
* publish nav only once for a date.
* clawback nav.
* reports.
* exchange rate table should have strategy_id.
* track currency gain. 
* historical dump processing.
* reconciliation of nav. -not nav(ledger) == nav(state) because you can't do nav(ledger)
  without positions. instead recon can be for smaller things like --
Cash from strategy state
==
Cash account balance in ledger

Total futures MTM
==
Sum of MTM ledger entries

Total management fees accrued
==
Management Fee Payable account

Total investor units
==
Outstanding units register

Position quantities
==
Net quantities implied by trade history
* max hardcoded stuff.
* create bill group api.
* get all bill groups api.
* link investor to billgroup api.
* apply perf fee api.
