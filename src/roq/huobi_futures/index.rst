.. _roq-huobi-futures:

.. |checkmark| unicode:: U+2713

roq-huobi-futures
=================

.. important::
   This gateway needs sponsorship to complete certain features.

.. important::
   There are different network end-points required for linear and inverse
   perpetuals/futures.
   The API's are different but sufficiently similar to allow this gateway to
   support both.
   The implication of this is that you will need more instances of this gateway
   if you need support for both product groups.



Links
-----

* `Website <https://futures.huobi.com/en-us/>`__
* `Support <https://huobiglobal.zendesk.com/hc/en-us>`__
* `API <https://huobiapi.github.io/docs/dm/v1/en/>`__


Purpose
-------

* Maintain network connectivity with the Huobi Futures exchange
* Route exchange updates to connected clients
* Route client requests to the relevant exchange accounts
* Stream all messages to an event-log


Overview
--------

.. grid::  2
  :gutter: 2

  .. grid-item-card::  Products

    .. list-table::
      :widths: auto

      * - Spot
        -
      * - Futures
        - |checkmark|
      * - Options
        -

  .. grid-item-card::  Market Data

    .. list-table::
      :widths: auto

      * - Reference Data
        - |checkmark|
      * - Market Status
        -
      * - Top of Book
        - |checkmark|
      * - Market by Price (L2)
        - |checkmark|
      * - Market by Order (L3)
        -
      * - Trade Summary
        - |checkmark|
      * - Statistics
        - |checkmark|

  .. grid-item-card::  Order Management

    .. list-table::
      :widths: auto

      * - Create
        - |checkmark|
      * - Modify
        -
      * - Cancel
        - |checkmark|
      * - Cancel All
        - |checkmark|
      * - Auto Cancellation
        -

  .. grid-item-card::  Account Management

    .. list-table::
      :widths: auto

      * - Positions
        - |checkmark|
      * - Funds
        - |checkmark|

* Data center located in Singapore (to be confirmed)
* No test environment


Conda
-----

* :ref:`Using Conda <tutorial-conda>`

.. tab:: Install

  .. code-block:: bash

    $ mamba install \
      --channel https://roq-trading.com/conda/stable \
      roq-huobi-futures

.. tab:: Configure

  .. code-block:: bash

    $ cp $CONDA_PREFIX/share/roq-huobi-futures/config.toml $CONFIG_FILE_PATH

    # Then modify $CONFIG_FILE_PATH to match your specific configuration

.. tab:: Run

  .. code-block:: bash

    $ roq-huobi-futures \
          --name "huobi-futures" \
          --config_file "$CONFIG_FILE_PATH" \
          --client_listen_address "$UNIX_SOCKET_PATH" \
          --service_listen_address "$TCP_LISTEN_PORT" \
          --flagfile "$FLAG_FILE"


Config
------

* :ref:`Common Config <gateway-config>`


Flags
-----

* :ref:`Using Flags <abseil-cpp>`
* :ref:`Common Flags <gateway-flags>`

.. code-block:: bash

   $ roq-huobi-futures --help

.. tab:: Rest

  .. list-table::
     :widths: auto

     * - :code:`--rest_download_refresh`
       - Periodic refresh

.. tab:: Flags

   .. include:: flags/flags.rstinc

.. tab:: Common

   .. include:: flags/common.rstinc

.. tab:: REST

   .. include:: flags/rest.rstinc

.. tab:: WS

   .. include:: flags/ws.rstinc


Environments
------------

.. code-block:: bash

  $ $CONDA_PREFIX/share/roq-huobi-futures/flags

COIN-M Futures
~~~~~~~~~~~~~~

.. tab:: Prod

   .. include:: flags/prod/flags.cfg
     :code: ini

.. note::
   You may also specify :code:`--api=""` (the default).

COIN-M Swaps
~~~~~~~~~~~~

.. tab:: Prod

   .. include:: flags/prod/flags-swap.cfg
     :code: ini


USDT-M
~~~~~~

.. tab:: Prod

   .. include:: flags/prod/flags-linear-swap.cfg
     :code: ini


Market Data
-----------

.. tab:: Live

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::ReferenceData`
      - DropCopy
      - public.$symbol.contract_info
      - Requires authentication.

    * - :cpp:class:`roq::MarketStatus`
      -
      -
      -

    * - :cpp:class:`roq::TopOfBook`
      - MarketData
      - market.$symbol.bbo
      -

    * - :cpp:class:`roq::MarketByPriceUpdate`
      - MarketData
      - market.$symbol.depth.size_150.high_freq
      -

    * - :cpp:class:`roq::MarketByOrderUpdate`
      -
      -
      -

    * - :cpp:class:`roq::TradeSummary`
      - MarketData
      - market.$symbol.trade.detail
      -

    * - :cpp:class:`roq::StatisticsUpdate`
      - MarketData
      - market.$symbol.detail, market.$symbol.estimated_rate.1min, market.$symbol.premium_index.1min, public.$symbol.funding_rate
      -

.. tab:: Download

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::ReferenceData`
      - Rest
      - /api/v1/contract_contract_info, /linear-swap-api/v1/swap_contract_info
      -

    * - :cpp:class:`roq::MarketStatus`
      -
      -
      -

    * - :cpp:class:`roq::TopOfBook`
      -
      -
      -

    * - :cpp:class:`roq::MarketByPriceUpdate`
      -
      -
      -

    * - :cpp:class:`roq::MarketByOrderUpdate`
      -
      -
      -

    * - :cpp:class:`roq::TradeSummary`
      -
      -
      -

    * - :cpp:class:`roq::StatisticsUpdate`
      -
      -
      -

Statistics
~~~~~~~~~~

.. list-table::
  :header-rows: 1
  :widths: auto

  * - Type
    - Comments

  * - :cpp:class:`OPEN_PRICE`
    - (detail) open

  * - :cpp:class:`HIGHEST_TRADED_PRICE`
    - (detail) high

  * - :cpp:class:`LOWEST_TRADED_PRICE`
    - (detail) low

  * - :cpp:class:`CLOSE_PRICE`
    - (detail) close

  * - :cpp:class:`TRADE_VOLUME`
    - (detail) vol

  * - :cpp:class:`FUNDING_RATE`
    - (funding_rate) funding_rate

  * - :cpp:class:`FUNDING_RATE_PREDICTION`
    - (funding_rate) estimated_rate

  * - :cpp:class:`INDEX_VALUE`
    - (index) close


Order Management
----------------

.. tab:: Live

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::OrderUpdate`
      -
      -
      -

    * - :cpp:class:`roq::TradeUpdate`
      -
      -
      -

.. tab:: Download

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::OrderUpdate`
      -
      -
      -

    * - :cpp:class:`roq::TradeUpdate`
      -
      -
      -

.. tab:: Request

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::CreateOrder`
      -
      -
      -

    * - :cpp:class:`roq::ModifyOrder`
      -
      -
      -

    * - :cpp:class:`roq::CancelOrder`
      -
      -
      -

    * - :cpp:class:`roq::CancelAllOrders`
      -
      -
      -

.. tab:: Response

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::OrderAck`
      -
      -
      -

Order Types
~~~~~~~~~~~

TBD


Time in Force
~~~~~~~~~~~~~

TBD


Position Effect
~~~~~~~~~~~~~~~

TBD


Execution Instructions
~~~~~~~~~~~~~~~~~~~~~~

TBD


Account Management
------------------

.. tab:: Live

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::PositionUpdate`
      -
      -
      -

    * - :cpp:class:`roq::FundsUpdate`
      -
      -
      -

.. tab:: Download

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::PositionUpdate`
      -
      -
      -

    * - :cpp:class:`roq::FundsUpdate`
      -
      -
      -


Streams
-------

.. tab:: OrderEntry

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Type
      - Comments

    * - REST
      - Primary purpose


Comments
--------

* Symbols are only processed when :code:`contract_status==1`
* The channels :code:`premium_index` and :code:`estimated_rate` are not available
  when :code:`--api` is :code:`inverse` (or missing, the default).
