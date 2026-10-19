PlatformIO
==========

.. dropdown:: Full Workflow
    :animate: fade-in-slide-down

    .. literalinclude:: ../../../../../.github/workflows/platformio.yml
        :language: yaml
        :linenos:

What It Does
------------

This workflow verifies firmware compilation using PlatformIO.


When It Does
------------

.. literalinclude:: ../../../../../.github/workflows/platformio.yml
    :language: yaml
    :start-after: name:
    :end-before: jobs:

This workflow runs whenever it's triggered by an user, whenever there is a push to the ``main`` branch that affects any firmware files and by other workflows.


How to... Verify Locally
------------------------

It's possible to locally evaluate the behavior with the following:

.. code-block:: bash

    curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.3/install.sh | bash

    npm install
    npx dotenv-cli -e .env -- npx semantic-release --dry-run
