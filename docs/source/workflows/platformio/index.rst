PlatformIO
==========

.. dropdown:: Full Workflow
    :animate: fade-in-slide-down

    .. literalinclude:: ../../../../.github/workflows/platformio.yml
        :language: yaml
        :linenos:

What It Does
------------

This workflow verifies firmware compilation using PlatformIO.


When It Does
------------

.. literalinclude:: ../../../../.github/workflows/platformio.yml
    :language: yaml
    :start-after: name:
    :end-before: jobs:

This workflow runs whenever it's triggered by an user, whenever there is a push to the ``main`` branch that affects any firmware files and by other workflows.
