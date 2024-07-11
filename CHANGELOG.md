# Change Log

All notable changes will be documented in this file.

## Head

## 1.0.2 &ndash; 2024-07-14

## 1.0.1 &ndash; 2024-04-14

## 1.0.0 &ndash; 2024-03-16

## 0.9.9 &ndash; 2024-01-28

## 0.9.8 &ndash; 2023-11-20

## 0.9.7 &ndash; 2023-09-18

## 0.9.6 &ndash; 2023-07-22

## 0.9.5 &ndash; 2023-06-12

## 0.9.4 &ndash; 2023-05-04

## 0.9.3 &ndash; 2023-03-20

## 0.9.2 &ndash; 2023-02-22

## 0.9.1 &ndash; 2023-01-12

## 0.9.0 &ndash; 2022-12-22

## 0.8.9 &ndash; 2022-11-14

## 0.8.8 &ndash; 2022-10-04

## 0.8.7 &ndash; 2022-08-22

## 0.8.6 &ndash; 2022-07-18

## 0.8.5 &ndash; 2022-06-06

### Added

* New flag to control periodic download of reference data (#233)

### Changed

* Market data support for `--net_disconnect_on_idle_timeout`.

### Fixed

* TradeSummary now reported as number of lots (#220)

## 0.8.4 &ndash; 2022-05-14

### Changed

* Extend ReferenceData with `min_trade_vol` and `trade_vol_step_size` (#216)

## 0.8.3 &ndash; 2022-03-22

## 0.8.2 &ndash; 2022-02-18

### Added

* Support funding rates (#172)
* Add funding rate and index price (#162)

## 0.8.1 &ndash; 2022-01-16

## 0.8.0 &ndash; 2022-01-12

## 0.7.9 &ndash; 2021-12-08

## 0.7.8 &ndash; 2021-11-02

### Added

* Add `max_trade_vol` and `trade_vol_step_size` to ReferenceData (#100)

### Changed

* Interface to support binary data from web::socket
* ReferenceData currencies should follow FX conventions (#99)
* Replace `snapshot` (bool) with `update_type` (UpdateType) (#97)
* Moved signature handling to tools library (chore)

### Removed

* Remove custom literals (#110)
* Remove external rate-limiter mirroring from the REST connection (#83)

## 0.7.7 &ndash; 2021-09-20

### Changed

* Added HTTP `request_id` (#55)

## 0.7.6 &ndash; 2021-09-02

### Changed

* New order management interface (#25)

## 0.7.5 &ndash; 2021-08-08

## 0.7.4 &ndash; 2021-07-20

## 0.7.3 &ndash; 2021-07-06

## 0.7.2 &ndash; 2021-06-20

## 0.7.1 &ndash; 2021-05-30
