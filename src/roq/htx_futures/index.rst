.. _roq-htx-futures:

.. |dagger| unicode:: U+2020
.. |double-dagger| unicode:: U+2021
.. |right-arrow| unicode:: U+2192
.. |right-double-arrow| unicode:: U+21D2
.. |left-right-double-arrow| unicode:: U+21D4
.. |check-mark| unicode:: U+2705
.. |cross-mark| unicode:: U+274C
.. |negative-cross-mark| unicode:: U+274E
.. |footnote-1| unicode:: U+2776
.. |footnote-2| unicode:: U+2777
.. |footnote-3| unicode:: U+2778


roq-htx-futures
===============


.. tab:: Unstable

  .. code-block:: shell

     $ conda install \
           --channel https://roq-trading.com/conda/unstable \
           roq-htx-futures

.. tab:: Stable

  .. code-block:: shell

     $ conda install \
           --channel https://roq-trading.com/conda/stable \
           roq-htx-futures


Supports
--------

.. grid::  2
  :gutter: 2

  .. grid-item-card::  Products

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:enumerator:`Spot <roq::SecurityType::SPOT>`
        - |cross-mark|
        -
      * - :cpp:enumerator:`Futures <roq::SecurityType::FUTURES>`
        - |check-mark|
        -
      * - :cpp:enumerator:`Swap <roq::SecurityType::SWAP>`
        - |check-mark|
        -
      * - :cpp:enumerator:`Option <roq::SecurityType::OPTION>`
        - |cross-mark|
        -

  .. grid-item-card::  Market Data

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`ReferenceData <roq::ReferenceData>`
        - |check-mark|
        - |footnote-1|
      * - :cpp:class:`MarketStatus <roq::MarketStatus>`
        - |cross-mark|
        -
      * - :cpp:class:`TopOfBook <roq::TopOfBook>`
        - |check-mark|
        -
      * - :cpp:class:`MarketByPrice <roq::MarketByPriceUpdate>`
        - |check-mark|
        -
      * - :cpp:class:`MarketByOrder <roq::MarketByOrderUpdate>`
        - |cross-mark|
        -
      * - :cpp:class:`TradeSummary <roq::TradeSummary>`
        - |check-mark|
        -
      * - :cpp:class:`Statistics <roq::StatisticsUpdate>`
        - |check-mark|
        -
      * - :cpp:class:`TimeSeries <roq::TimeSeriesUpdate>`
        - |cross-mark|
        -

  .. grid-item-card::  Orders & Quotes

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`CreateOrder <roq::CreateOrder>`
        - |negative-cross-mark|
        -
      * - :cpp:class:`ModifyOrder <roq::ModifyOrder>`
        - |cross-mark|
        -
      * - :cpp:class:`CancelOrder <roq::CancelOrder>`
        - |negative-cross-mark|
        -
      * - :cpp:class:`CancelAllOrders <roq::CancelAllOrders>`
        - |negative-cross-mark|
        -
      * - :cpp:class:`MassQuote <roq::MassQuote>`
        - |cross-mark|
        -
      * - :cpp:class:`CancelQuotes <roq::CancelQuotes>`
        - |cross-mark|
        -

  .. grid-item-card::  Account

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`Funds <roq::FundsUpdate>`
        - |negative-cross-mark|
        -
      * - :cpp:class:`Position <roq::PositionUpdate>`
        - |negative-cross-mark|
        -

.. note::

   |check-mark| = Available.

   |negative-cross-mark| = Not implemented.

   |cross-mark| = Unavailable.

   |footnote-1| The exchange protocol does not support streaming updates for reference data and market status.


Using
-----

.. code-block:: shell

   $ roq-htx-futures [FLAGS]


.. _roq-htx-futures-flags:

Flags
-----

.. code-block:: shell

   $ roq-htx-futures --help

.. tab:: Flags

   .. include:: flags/flags.rstinc

.. tab:: REST

   .. include:: flags/rest.rstinc

.. tab:: WS

   .. include:: flags/ws.rstinc

.. tab:: Misc

   .. include:: flags/misc.rstinc


Environments
------------

.. tab:: Prod (USDT-M-FUTURES)

   .. code-block:: shell

      $ --flagfile $CONDA_PREFIX/share/roq-htx-futures/flags/prod/flags-usdt-m-futures.cfg

   .. include:: flags/prod/flags-usdt-m-futures.cfg
     :code: shell

.. tab:: Prod (COIN-M-DELIVERY)

   .. code-block:: shell

      $ --flagfile $CONDA_PREFIX/share/roq-htx-futures/flags/prod/flags-coin-m-delivery.cfg

   .. include:: flags/prod/flags-coin-m-delivery.cfg
     :code: shell

.. tab:: Prod (COIN-M-PERPETUAL)

   .. code-block:: shell

      $ --flagfile $CONDA_PREFIX/share/roq-htx-futures/flags/prod/flags-coin-m-perpetual.cfg

   .. include:: flags/prod/flags-coin-m-perpetual.cfg
     :code: shell


Configuration
-------------

.. code-block:: shell

   $ --config_file $CONDA_PREFIX/share/roq-htx-futures/config.toml

.. important::

   This template will be replaced when the software is upgraded.
   Make a copy and modify to your own needs.

.. include:: config.toml
   :code: toml



Market Data
-----------

Inbound
~~~~~~~

.. tab:: StatisticsType

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - Event
       - Field
       -
       -

     * - :code:`detail`
       - :code:`open`
       - |right-double-arrow|
       - :cpp:enumerator:`OPEN_PRICE <roq::StatisticsType::OPEN_PRICE>`

     * - :code:`detail`
       - :code:`high`
       - |right-double-arrow|
       - :cpp:enumerator:`HIGHEST_TRADED_PRICE <roq::StatisticsType::HIGHEST_TRADED_PRICE>`

     * - :code:`detail`
       - :code:`low`
       - |right-double-arrow|
       - :cpp:enumerator:`LOWEST_TRADED_PRICE <roq::StatisticsType::LOWEST_TRADED_PRICE>`

     * - :code:`detail`
       - :code:`close`
       - |right-double-arrow|
       - :cpp:enumerator:`CLOSE_PRICE <roq::StatisticsType::CLOSE_PRICE>`

     * - :code:`detail`
       - :code:`vol`
       - |right-double-arrow|
       - :cpp:enumerator:`TRADE_VOLUME <roq::StatisticsType::TRADE_VOLUME>`

     * - :code:`index`
       - :code:`close`
       - |right-double-arrow|
       - :cpp:enumerator:`INDEX_VALUE <roq::StatisticsType::INDEX_VALUE>`

     * - :code:`funding_rate`
       - :code:`funding_rate`
       - |right-double-arrow|
       - :cpp:enumerator:`FUNDING_RATE <roq::StatisticsType::FUNDING_RATE>`

     * - :code:`funding_rate`
       - :code:`estimated_rate`
       - |right-double-arrow|
       - :cpp:enumerator:`FUNDING_RATE_PREDICTION <roq::StatisticsType::FUNDING_RATE_PREDICTION>`


Order Management
----------------


Inbound
~~~~~~~


Outbound
~~~~~~~~


Comments
--------

* Symbols are only processed when :code:`contract_status==1`
* The channels :code:`premium_index` and :code:`estimated_rate` are not available
  when :code:`--api` is one of :code:`USDT-M-FUTURES`, :code:`COIN-M-DELIVERY`, :code:`COIN-M-PERPETUAL`.


References
----------


Common
~~~~~~

* :ref:`Using Conda <tutorial-conda>`
* :ref:`Using Flags <abseil-cpp>`
* :ref:`Gateway Flags <gateway-flags>`
* :ref:`Gateway Config <gateway-config>`


Exchange
~~~~~~~~

* `Website <https://www.htx.com/futures>`__
* `Documentation <https://www.htx.com/en-us/opend/newApiPages/>`__
